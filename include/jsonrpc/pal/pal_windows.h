#pragma once
#include <boost/asio.hpp>

#include "Windows.h"

namespace lsplex::jsonrpc::pal {

namespace asio = boost::asio;
using asio::write;
using asio::readable_pipe;
using asio::stream_file;
using asio::writable_pipe;
using asio::windows::stream_handle;

namespace detail {
  inline std::string get_error_msg(const std::string& msg) {
    return msg + ": windows reasonz";  // doing this properly too gross for now
  }
}  // namespace detail

struct readable_file : stream_file {
  template <typename Executor>
  readable_file(Executor&& ex, const std::string& path)
      : stream_file{std::forward<Executor>(ex), path,
                    boost::asio::file_base::read_only} {}
};

struct asio_stdin : boost::asio::readable_pipe {
  template <typename Executor> explicit asio_stdin(Executor&& ex) // NOLINT
      : readable_pipe{std::forward<Executor>(ex)} {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE)
      throw std::runtime_error(detail::get_error_msg("GetStdHandle() failed"));

    writable_pipe wp{std::forward<Executor>(ex)};
    connect_pipe(*this, wp);

    std::thread([h = h, wp = std::move(wp)]() mutable {
      std::array<char, 1024> buffer{};

      for (;;) {
        DWORD bytesRead = 0;
        BOOL result
          = ReadFile(h, buffer.data(), static_cast<DWORD>(buffer.size()),
                       &bytesRead, nullptr);
        if (!result) {
          auto msg = detail::get_error_msg("ReadFile() failed");
          throw std::runtime_error(msg);
        }
        if (bytesRead > 0)
          asio::write(wp, asio::buffer(buffer.data(), bytesRead));
        else
          break; // bytesRead == 0 means EOF
      }

    }).detach();
  }
};

struct asio_stdout : stream_handle {
  template <typename Executor> explicit asio_stdout(Executor&& ex) // NOLINT
      : stream_handle{std::forward<Executor>(ex),  // Nope, it doesn't work.
                      GetStdHandle(STD_OUTPUT_HANDLE)} {}
};

}  // namespace lsplex::jsonrpc::pal
