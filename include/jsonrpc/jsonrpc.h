#pragma once

#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <memory_resource>
#include <regex>
#include <sstream>

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
template <typename Readable> LSPLEX_EXPORT class istream {
  Readable _in;
  asio::streambuf _header_buf{30};
  std::vector<char> _msg_buf;

public:
  LSPLEX_EXPORT Readable& handle() { return _in; }
  LSPLEX_EXPORT explicit istream(Readable d) : _in{std::move(d)} {}
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
template <typename Writeable> LSPLEX_EXPORT class ostream {
  Writeable _out;

public:
  LSPLEX_EXPORT Writeable& handle() { return _out; }
  LSPLEX_EXPORT explicit ostream(Writeable d) : _out{std::move(d)} {}
  LSPLEX_EXPORT asio::awaitable<void> async_put(const json::object& o);
  LSPLEX_EXPORT void put(const json::object& o) {
    return asio::co_spawn(_out.get_executor(), async_put(o), asio::use_future)
        .get();
  }
};
}  // namespace lsplex::jsonrpc

// Implementation: maybe move this to a different file

namespace lsplex::jsonrpc::detail {

namespace asio = boost::asio;

struct parse_state {
  size_t content_length = 0;
  size_t erledigt = 0;
  bool at_eol{false};
  size_t consume{0};
};

struct parse_mandatory_headers {
  explicit parse_mandatory_headers(parse_state* ps) : pstate(ps) {}
  parse_state* pstate;
  using It = asio::buffers_iterator<asio::streambuf::const_buffers_type>;

  std::pair<It, bool> operator()(It begin, It end) const {
    if (begin == end) return std::make_pair(begin, false);
    if (*begin == '\n') begin++;

    while (true) {
      constexpr std::string_view eol{"\r\n"};
      constexpr std::string_view magic{"Content-Length"};

      It eol_probe{begin};
      for (char c : eol) {
        eol_probe = std::find(eol_probe, end, c);
        if (eol_probe == end) {
          pstate->at_eol = false;
          return std::make_pair(begin, false);
        }
        pstate->consume = static_cast<size_t>(eol_probe - begin);
        eol_probe++;
      }

      pstate->consume = static_cast<size_t>(eol_probe - begin);

      if (pstate->content_length != 0  // If got mandatory header
          && pstate->at_eol            // and last saw a complete line
          && eol_probe - static_cast<ptrdiff_t>(eol.size())
                 == begin              // and looking at another CRLF...
      ) {
        // ...we're about done!
        return std::make_pair(eol_probe, true);
      }
      pstate->at_eol = true;

      // Check if \r\n terminated line is header
      std::regex header_re{R"(([^ ]+)\s*:\s*([^ ]+))"};
      std::array<char, 512> buf{};
      std::pmr::monotonic_buffer_resource resource{buf.data(), buf.size()};
      std::pmr::polymorphic_allocator<std::sub_match<It>> alloc{&resource};
      std::pmr::match_results<It> match{alloc};

      if (std::regex_match(begin, eol_probe, match, header_re)) {
        if (std::equal(match[1].first, match[1].second, magic.begin())) {
          for (It cp = match[2].first;
               cp != match[2].second && static_cast<bool>(isdigit(*cp)); ++cp)
            pstate->content_length
                = pstate->content_length * 10 + static_cast<size_t>(*cp - '0');
        }
      }
      begin = eol_probe;
    }
  }
};

}  // namespace lsplex::jsonrpc::detail

namespace boost::asio {
template <>
struct is_match_condition<lsplex::jsonrpc::detail::parse_mandatory_headers>
    : public boost::true_type {};
}  // namespace boost::asio

namespace lsplex::jsonrpc {

constexpr auto use_nothrow_awaitable = asio::as_tuple(asio::use_awaitable);

template <typename Readable>
[[nodiscard]] asio::awaitable<json::object> istream<Readable>::async_get() {
  detail::parse_state state;
again:
  auto [ec, read] = co_await asio::async_read_until(
      _in, _header_buf, detail::parse_mandatory_headers{&state},
      use_nothrow_awaitable);
  if (ec) {
    if (ec == asio::error::misc_errors::not_found) {
      // The too small buffer is guaranteed full after a 'not_found'.
      // So consume something (else infloop).
      _header_buf.consume(state.consume > 0 ? state.consume
                                            : _header_buf.size());
      state.consume = 0;
      goto again;  // NOLINT
    }
    _header_buf.consume(_header_buf.size());
    throw boost::system::system_error{ec};
  }

  _header_buf.consume(read);
  _msg_buf.clear();
  _msg_buf.resize(state.content_length);

  auto sz = _header_buf.size();
  auto from = asio::buffers_begin(_header_buf.data());
  auto to
      = from + static_cast<std::ptrdiff_t>(std::min(state.content_length, sz));

  std::copy(from, to, _msg_buf.data());
  _header_buf.consume(static_cast<size_t>(to - from));

  if (sz < state.content_length)
    co_await asio::async_read(
        _in, asio::buffer(&_msg_buf[sz], state.content_length - sz),
        asio::use_awaitable);

  co_return json::parse(std::string_view{_msg_buf.data(), _msg_buf.size()})
      .as_object();
}

template <typename Writable>
asio::awaitable<void> ostream<Writable>::async_put(const json::object& o) {
  const auto s = json::serialize(o);  // FIXME: find a more eff
  std::stringstream header;
  header << "Content-Length: " << s.size() << "\r\n\r\n";
  co_await asio::async_write(_out, asio::buffer(header.str()),
                             asio::use_awaitable);
  co_await asio::async_write(_out, asio::buffer(s), asio::use_awaitable);
  co_return;
}
}  // namespace lsplex::jsonrpc
