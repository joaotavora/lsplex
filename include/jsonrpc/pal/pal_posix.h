#pragma once

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect_pipe.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/asio/write.hpp>
#include <stdexcept>
#include <thread>

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <vector>

namespace lsplex::jsonrpc::pal {

namespace asio = boost::asio;
using asio::posix::stream_descriptor;
using asio::readable_pipe;
using asio::writable_pipe;
using asio::connect_pipe;


namespace detail {
class fd {
  int _des = -1;
public:
  explicit fd(int des) : _des(des) {}
  fd(const fd& other) = delete;
  fd& operator= (const fd& other) = delete;
  fd(fd&& other) noexcept : _des(other._des) { other._des = -1; }
  fd& operator=(fd&& other) noexcept {
    if (this == &other) return *this;
    if (this != &other) {
      if (_des != -1) ::close(_des);
      _des = other._des;
      other._des = -1;
    }
    return *this;
  }
  ~fd() { if (_des != -1) ::close(_des); }
  operator int() const { return _des; } // NOLINT
};

inline std::string get_error_msg(const std::string& msg) {
  return msg; // too hard to make a portable strerror_r
}

}  // namespace detail

struct readable_file : readable_pipe {
  template <typename Executor>
  explicit readable_file (Executor&& ex, const std::string& path) :
    readable_pipe{std::forward<Executor>(ex)} {
    detail::fd file_fd{ ::open(path.c_str(), O_RDONLY | O_CLOEXEC)};
    if (file_fd == -1)
      throw std::runtime_error(detail::get_error_msg("::open() failed"));
    writable_pipe wp{std::forward<Executor>(ex)};
    connect_pipe(*this, wp);

    std::thread([file_fd=std::move(file_fd), wp=std::move(wp)]() mutable {
      std::array<char, 1024> buffer{};
      ssize_t bytes_read{};
    retry:
      while ((bytes_read = ::read(file_fd, buffer.data(), buffer.size())) > 0)
        asio::write(wp, asio::buffer(buffer));

      if (bytes_read == -1) {
        if (errno == EAGAIN) goto retry;  // NOLINT
        auto msg = detail::get_error_msg("::read() failed");
        throw std::runtime_error(msg);
      }
      // bytes_read == 0 means EOF
    }).detach();
  }
};

struct asio_stdin : stream_descriptor {
  template <typename Executor> explicit asio_stdin(Executor&& ex)  // NOLINT
      : stream_descriptor{std::forward<Executor>(ex),
                          fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC)} {}
};

struct asio_stdout : stream_descriptor {
  template <typename Executor> explicit asio_stdout(Executor&& ex) // NOLINT
      : stream_descriptor{std::forward<Executor>(ex),
                          fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC)} {}
};

class redirector {
public:
  redirector(const redirector &) = delete;
  redirector(redirector &&) = delete;
  redirector &operator=(const redirector &) = delete;
  redirector &operator=(redirector &&) = delete;

  explicit redirector(const std::string &inputFile)
      : orig_stdin{dup(STDIN_FILENO)}, orig_stdout{dup(STDOUT_FILENO)},
        file{open(inputFile.c_str(), O_RDONLY)} {
    if (file == -1)
      throw std::runtime_error("Failed to open input file");

    if (dup2(file, STDIN_FILENO) == -1)
      throw std::runtime_error("Failed to redirect stdin");

    if (pipe(pipe_fd) == -1)
      throw std::runtime_error("Failed to create pipe");

    if (dup2(pipe_fd[1], STDOUT_FILENO) == -1)
      throw std::runtime_error("Failed to redirect stdout");

    // Close the write end of the pipe in the parent process
    close(pipe_fd[1]);
  }

  ~redirector() {
    // Restore the original stdin and stdout file descriptors
    dup2(orig_stdin, STDIN_FILENO);
    dup2(orig_stdout, STDOUT_FILENO);

    // Close the original and new file descriptors
    close(orig_stdin);
    close(orig_stdout);
    close(file);
    close(pipe_fd[0]);
  }

  std::string slurp() {
    std::vector<char> buffer(1024);
    std::string output;
    ssize_t bytesRead = 0;;

    // Read the contents of the read end of the pipe
    while ((bytesRead = read(pipe_fd[0], buffer.data(), buffer.size())) > 0) {
      output.append(buffer.data(), static_cast<size_t>(bytesRead));
    }

    return output;
  }

private:
  int orig_stdin;
  int orig_stdout;
  int file;
  int pipe_fd[2]; // [0] is the read end, [1] is the write end
};
}  // namespace lsplex::jsonrpc::pal
