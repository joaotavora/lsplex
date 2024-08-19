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

namespace detail {
struct parse_state {
  size_t content_length = 0;
  size_t erledigt = 0;
  bool at_eol{false};
  size_t consume{0};
};
}  // namespace detail

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
  detail::parse_state _state;

public:
  LSPLEX_EXPORT Readable& handle() { return _in; }
  LSPLEX_EXPORT explicit istream(Readable d) : _in{std::move(d)} {}
  template <typename Token> LSPLEX_EXPORT auto async_get(Token&& tok);
  LSPLEX_EXPORT [[nodiscard]] json::object get() {
    return async_get(asio::use_future).get();
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
  std::string _out_buf;

public:
  LSPLEX_EXPORT Writeable& handle() { return _out; }
  LSPLEX_EXPORT explicit ostream(Writeable d) : _out{std::move(d)} {}
  template <typename Token>
  LSPLEX_EXPORT auto async_put(const json::object& o, Token&& tok);
  LSPLEX_EXPORT void put(const json::object& o) {
    async_put(o, asio::use_future).get();
  }
};
}  // namespace lsplex::jsonrpc

// Implementation: maybe move this to a different file

namespace lsplex::jsonrpc::detail {

namespace asio = boost::asio;

struct parse_mandatory_headers {
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
                 == begin  // and looking at another CRLF...
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

template <typename Readable> struct read_op {
  Readable& in;                 // NOLINT
  asio::streambuf& header_buf;  // NOLINT
  std::vector<char>& msg_buf;   // NOLINT
  detail::parse_state& state;   // NOLINT
  enum { starting, reading_header, reading_body } stage = starting;
  detail::parse_mandatory_headers parser{};

  template <typename Self> void operator()(Self& self,
                                           boost::system::error_code ec = {},
                                           std::size_t bread = 0) {
    switch (stage) {
      case starting: {
        state = {};
      again:
        stage = reading_header;
        asio::async_read_until(in, header_buf,
                               detail::parse_mandatory_headers{&state},
                               std::move(self));
        return;
      }
      case reading_header: {
        if (ec) {
          if (ec == asio::error::misc_errors::not_found) {
            header_buf.consume(state.consume > 0 ? state.consume
                                                 : header_buf.size());
            state.consume = 0;
            goto again;  // NOLINT
          }
          header_buf.consume(header_buf.size());
          self.complete(ec, {});
          return;
        }
        header_buf.consume(bread);
        msg_buf.clear();
        msg_buf.resize(state.content_length);
        auto sz = header_buf.size();
        auto from = asio::buffers_begin(header_buf.data());
        auto to
            = from
              + static_cast<std::ptrdiff_t>(std::min(state.content_length, sz));
        std::copy(from, to, msg_buf.data());
        header_buf.consume(static_cast<size_t>(to - from));

        if (sz < state.content_length) {
          stage = reading_body;
          asio::async_read(
              in, asio::buffer(&msg_buf[sz], state.content_length - sz),
              std::move(self));
          return;
        }
        goto done;  // NOLINT
      }
      case reading_body: {
        if (ec) {
          self.complete(ec, {});
          return;
        }
      done:
        self.complete(
            {}, json::parse(std::string_view{msg_buf.data(), msg_buf.size()})
                    .as_object());
      }
    }
  }
};

template <typename Writable> struct write_op {
  Writable& out;          // NOLINT
  const json::object& o;  // NOLINT
  std::string& out_buf;   // NOLINT

  enum { starting, writing_header, writing_body } stage = starting;

  template <typename Self>
  void operator()(Self& self, boost::system::error_code ec = {},
                  [[maybe_unused]] size_t written = 0) {
    switch (stage) {
      case starting: {
        out_buf = json::serialize(o);
        std::stringstream header;
        header << "Content-Length: " << out_buf.size() << "\r\n\r\n";
        stage = writing_header;
        asio::async_write(out, asio::buffer(header.str()), std::move(self));
        return;
      }
      case writing_header: {
        if (ec) {
          self.complete(ec);
          return;
        }
        stage = writing_body;
        asio::async_write(out, asio::buffer(out_buf), std::move(self));
        return;
      }
      case writing_body: {
        if (ec) {
          self.complete(ec);
          return;
        }
        self.complete({});
        return;
      }
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
template <typename Readable> template <typename Token>
[[nodiscard]] auto istream<Readable>::async_get(Token&& tok) {
  return asio::async_compose<Token, void(boost::system::error_code,
                                         boost::json::object)>(
      detail::read_op{_in, _header_buf, _msg_buf, _state}, tok, _in);
}
template <typename Writable> template <typename Token>
[[nodiscard]] auto ostream<Writable>::async_put(const json::object& o,
                                                Token&& tok) {
  return asio::async_compose<Token, void(boost::system::error_code)>(
      detail::write_op{_out, o, _out_buf}, tok, _out);
}
}  // namespace lsplex::jsonrpc
