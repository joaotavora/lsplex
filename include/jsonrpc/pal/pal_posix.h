#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect_pipe.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/asio/write.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace lsplex::jsonrpc::pal {

namespace asio = boost::asio;
using asio::connect_pipe;
using asio::readable_pipe;
using asio::writable_pipe;
using asio::posix::stream_descriptor;

namespace detail {
  class fd {
    int _des = -1;

  public:
    explicit fd(int des) : _des(des) {}
    fd(const fd& other) = delete;
    fd& operator=(const fd& other) = delete;
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
    ~fd() {
      if (_des != -1) ::close(_des);
    }
    operator int() const { return _des; }  // NOLINT
  };

  inline std::string get_error_msg(const std::string& msg) {
    return msg;  // too hard to make a portable strerror_r
  }

}  // namespace detail

class readable_file : public readable_pipe {
  writable_pipe _wp;
  detail::fd _file_fd;
  std::jthread _t;

  auto init_writable_pipe() {
    writable_pipe wp{this->get_executor()};
    connect_pipe(*this, wp);
    return wp;
  }

  static auto init_fd(const std::string& path) {
    detail::fd fd{::open(path.c_str(), O_RDONLY | O_CLOEXEC)};
    if (fd == -1)
      throw std::runtime_error(detail::get_error_msg("::open() failed"));
    return fd;
  }

  auto init_thread() {
    return std::jthread{[fd=std::move(_file_fd), wp=std::move(_wp)]() mutable {
      std::array<char, 1024> buffer{};
      ssize_t bytes_read{};
    retry:
      while ((bytes_read = ::read(fd, buffer.data(), buffer.size())) > 0)
        asio::write(wp, asio::buffer(buffer));

      if (bytes_read == -1) {
        if (errno == EAGAIN) goto retry;  // NOLINT
        auto msg = detail::get_error_msg("::read() failed");
        throw std::runtime_error(msg);
      }
    }};
  }
public:
  template <typename Executor>
  explicit readable_file(Executor&& ex, const std::string& path)
      : readable_pipe{std::forward<Executor>(ex)},
        _wp{init_writable_pipe()},
        _file_fd{init_fd(path)},
        _t{init_thread()} {}
};

class asio_stdin : public readable_pipe {
  writable_pipe _wp;
  detail::fd _file_fd;
  std::jthread _t;

  auto init_writable_pipe() {
    writable_pipe wp{this->get_executor()};
    connect_pipe(*this, wp);
    return wp;
  }

  static auto init_fd() {
    detail::fd fd{::dup(STDIN_FILENO)};
    if (fd == -1)
      throw std::runtime_error(detail::get_error_msg("::dup() failed"));
    return fd;
  }

  auto init_thread() {
    return std::jthread{[fd=std::move(_file_fd), wp=std::move(_wp), this]() mutable {
      std::array<char, 1024> buffer{};
      ssize_t bytes_read{};
    retry:
      while ((bytes_read = ::read(fd, buffer.data(), buffer.size())) > 0){
        asio::write(wp, asio::buffer(buffer, static_cast<size_t>(bytes_read)));
      }

      if (bytes_read == -1) {
        if (errno == EAGAIN) goto retry;  // NOLINT
        auto msg = detail::get_error_msg("::read() failed");
        throw std::runtime_error(msg);
      }
      wp.close();
    }};
  }
public:
template <typename Executor>
  explicit asio_stdin(Executor&& ex) // NOLINT
      : readable_pipe{std::forward<Executor>(ex)},
        _wp{init_writable_pipe()},
        _file_fd{init_fd()},
        _t{init_thread()} {}
};

struct asio_stdout : stream_descriptor {
  template <typename Executor> explicit asio_stdout(Executor&& ex)  // NOLINT
      : stream_descriptor{std::forward<Executor>(ex),
                          fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC)} {}
};

class redirector {
public:
  redirector(const redirector&) = delete;
  redirector(redirector&&) = delete;
  redirector& operator=(const redirector&) = delete;
  redirector& operator=(redirector&&) = delete;

  explicit redirector(const std::string& inputFile)
      : orig_stdin{fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC)},
        orig_stdout{fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC)},
        file{open(inputFile.c_str(), O_RDONLY | O_CLOEXEC)} {
    if (file == -1) throw std::runtime_error("Failed to open input file");

    if (dup2(file, STDIN_FILENO) == -1)
      throw std::runtime_error("Failed to redirect stdin");

    if (pipe2(pipe_fd.data(), O_CLOEXEC) == -1)
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
    ssize_t bytesRead = 0;
    ;

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
  std::array<int, 2> pipe_fd{};  // [0] is the read end, [1] is the write end
};
}  // namespace lsplex::jsonrpc::pal
