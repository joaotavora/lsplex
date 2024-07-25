#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/json/object.hpp>

#include "lsplex/export.hpp"

namespace lsplex::jsonrpc {

namespace json = boost::json;
namespace asio = boost::asio;

/** HTTP-like way to stream in JSON objects from a file descriptor.
 *
 *  See https://microsoft.github.io/language-server-protocol/
 *             specifications/lsp/3.17/specification/#headerPart
 *
 * Also FIXME find better name for this.
 */
LSPLEX_EXPORT class istream {
  asio::posix::stream_descriptor _in;
  asio::streambuf _header_buf{30};
  std::vector<char> _msg_buf;

public:
  LSPLEX_EXPORT explicit istream(asio::posix::stream_descriptor d)
      : _in{std::move(d)} {}
  LSPLEX_EXPORT asio::awaitable<json::object> async_get();
  LSPLEX_EXPORT [[nodiscard]] json::object get() {
    return asio::co_spawn(_in.get_executor(), async_get(), asio::use_future)
        .get();
  }
};

/** HTTP-like way to stream out JSON objects to a file descriptor.
 *
 *  See https://microsoft.github.io/language-server-protocol/
 *             specifications/lsp/3.17/specification/#headerPart
 *
 * Also FIXME find better name for this.
 */
LSPLEX_EXPORT class ostream {
  asio::posix::stream_descriptor _out;

public:
  LSPLEX_EXPORT explicit ostream(asio::posix::stream_descriptor d)
      : _out{std::move(d)} {}
  LSPLEX_EXPORT asio::awaitable<void> async_put(const json::object& o);
  LSPLEX_EXPORT void put(const json::object& o) {
    return asio::co_spawn(_out.get_executor(), async_put(o), asio::use_future)
        .get();
  }
};
}  // namespace lsplex::jsonrpc
