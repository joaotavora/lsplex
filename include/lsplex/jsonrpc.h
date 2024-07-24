#pragma once

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/json/object.hpp>

#include "lsplex/export.hpp"

namespace lsplex::jsonrpc {

namespace json = boost::json;
namespace asio = boost::asio;

LSPLEX_EXPORT class istream {
  asio::posix::stream_descriptor _in;
  asio::streambuf _header_buf{30};
  std::vector<char> _msg_buf;

  asio::awaitable<json::object> get_1();

public:
  LSPLEX_EXPORT explicit istream(asio::posix::stream_descriptor d) : _in{std::move(d)} {}
  LSPLEX_EXPORT [[nodiscard]] json::object get() {
    return asio::co_spawn(_in.get_executor(), get_1(), asio::use_future).get();
  }
};
}  // namespace lsplex::jsonrpc
