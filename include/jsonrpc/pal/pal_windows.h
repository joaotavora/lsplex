#pragma once

#include <fmt/core.h>

#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/winapi/file_management.hpp>
#include <thread>

namespace lsplex::jsonrpc::pal {

namespace asio = boost::asio;
using asio::readable_pipe;
using asio::stream_file;
using asio::writable_pipe;
using asio::write;
using asio::windows::stream_handle;

namespace detail {

  inline std::string get_error_msg(const std::string& msg) {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) return "no error";

    char buf[256];  // NOLINT
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &buf[0],
        sizeof(buf), nullptr);

    return msg + ":" + std::string(&buf[0], size);
  }
}  // namespace detail

struct readable_file : stream_file {
  template <typename Executor>
  readable_file(Executor&& ex, const std::string& path)
      : stream_file{std::forward<Executor>(ex), path,
                    boost::asio::file_base::read_only} {}
};

class asio_stdin : public readable_pipe {
  writable_pipe _wp;
  std::jthread _t;

  writable_pipe init_writable_pipe() {
    writable_pipe wp{this->get_executor()};
    connect_pipe(*this, wp);
    return wp;
  }

  std::jthread init_thread() {
    return std::jthread([wp = std::move(_wp)]() mutable {
      HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
      if (h == INVALID_HANDLE_VALUE)
        throw std::runtime_error(
            detail::get_error_msg("GetStdHandle() failed"));
      std::array<char, 1024> buffer{};

      for (;;) {
        DWORD bread = 0;
        BOOL res = ReadFile(h, buffer.data(), buffer.size(), &bread, nullptr);
        if (res && bread == 0) {
          wp.close();
          break;
        }
        if (!res) {
          if (::GetLastError() == ERROR_BROKEN_PIPE) {
            wp.close();
            break;
          }
          auto msg = detail::get_error_msg("ReadFile() failed");
          throw std::runtime_error(msg);
        }
        asio::write(wp, asio::buffer(buffer.data(), bread));
      }
    });
  }

public:
  template <typename Executor> explicit asio_stdin(Executor&& ex)  // NOLINT
      : readable_pipe{std::forward<Executor>(ex)},
        _wp(init_writable_pipe()),
        _t(init_thread()) {}
};

class asio_stdout : public writable_pipe {
  readable_pipe _rp;
  std::jthread _t;

  readable_pipe init_readable_pipe() {
    readable_pipe rp{this->get_executor()};
    connect_pipe(rp, *this);
    return rp;
  }

  std::jthread init_thread() {
    return std::jthread{[rp = std::move(_rp)]() mutable {
      std::array<char, 512> buffer{};

      boost::system::error_code ec;

      HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
      if (h == INVALID_HANDLE_VALUE)
        throw std::runtime_error(
            detail::get_error_msg("GetStdHandle() failed"));

      for (;;) {
        auto bread
            = rp.read_some(asio::buffer(buffer.data(), buffer.size()), ec);
        if (!ec) {
          auto to_write = bread;
          while (to_write > 0) {
            DWORD written = 0;
            BOOL result = boost::winapi::WriteFile(
                h, buffer.data() + written,
                static_cast<DWORD>(to_write - written), &written, nullptr);
            to_write -= written;
            if (!result)
              throw std::runtime_error(
                  detail::get_error_msg("WriteFile() failed"));
          }
        } else if (ec == asio::error::eof || ec == asio::error::broken_pipe) {
          // fmt::println(stderr, "child stdout closed");
          break;
        } else
          throw std::runtime_error(
              detail::get_error_msg("asio::read() failed"));
      }
    }};
  }

public:
  template <typename Executor> explicit asio_stdout(Executor&& ex)  // NOLINT
      : writable_pipe{std::forward<Executor>(ex)},
        _rp{init_readable_pipe()},
        _t{init_thread()} {}
};

}  // namespace lsplex::jsonrpc::pal
