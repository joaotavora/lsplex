#pragma once
#include <boost/asio.hpp>
#include "Windows.h"

namespace lsplex::jsonrpc::pal {

using boost::asio::stream_file;
using boost::asio::windows::stream_handle;

struct readable_file : stream_file {
  template <typename Executor>
  readable_file(Executor&& ex, const std::string& path)
      : stream_file{std::forward<Executor>(ex), path,
                    boost::asio::file_base::read_only} {}
};

struct asio_stdin : stream_handle {
  template <typename Executor> explicit asio_stdin(Executor&& ex)
      : stream_handle{std::forward<Executor>(ex),  // Nope, it doesn't work.
                      GetStdHandle(STD_INPUT_HANDLE)} {}
};

struct asio_stdout : stream_handle {
  template <typename Executor> explicit asio_stdout(Executor&& ex)
      : stream_handle{std::forward<Executor>(ex),  // Nope, it doesn't work.
                      GetStdHandle(STD_OUTPUT_HANDLE)} {}
};

}  // namespace lsplex::jsonrpc::pal
