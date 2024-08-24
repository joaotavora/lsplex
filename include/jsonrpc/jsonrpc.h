#pragma once

#include <boost/asio.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/json.hpp>
#include <memory_resource>
#include <regex>
#include <sstream>
#include <utility>

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
  asio::streambuf _buf{30};

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

template <typename Readable> class read_op {
  Readable& _in;          // NOLINT
  asio::streambuf& _buf;  // NOLINT

  std::vector<char> _msg_buf{};  // NOLINT
  enum { starting, parse_headers, reading_body } stage = starting;
  using it_t = asio::buffers_iterator<asio::streambuf::const_buffers_type>;
  size_t _content_length{0};

public:
  read_op(Readable& in, asio::streambuf& buf) : _in{in}, _buf{buf} {}

  // NOLINTNEXTLINE(*-cognitive-complexity)
  template <typename Self> void operator()([[maybe_unused]] Self& self,
                                           [[maybe_unused]] boost::system::error_code ec = {},
                                           [[maybe_unused]] std::size_t bread = 0) {
    switch (stage) {
      case starting: {
        stage = parse_headers;
      again:
        _in.async_read_some(_buf, std::move(self));
        return;
      }
      case parse_headers: {
        if (bread == 0) {
          self.complete(asio::error::misc_errors::eof, {});
          return;
        }
        for (;;) {
          auto beg = asio::buffers_begin(_buf.data());
          auto end = asio::buffers_end(_buf.data());
          constexpr std::string_view searcher{"\r\n"};
          auto crlf = std::search(beg, end, searcher.begin(), searcher.end());

          if (crlf == end) {
            // no crlf in sight, need to get more data, possibly
            // emptying the buffer to make space for it.
            if (_buf.size() == _buf.max_size()) _buf.consume(_buf.size());
            goto again;  // NOLINT
          }

          if (_content_length != 0 && crlf == beg) break;


          std::regex header_re{R"(([^ ]+)\s*:\s*([^ ]+))"};
          std::array<char, 512> buf{};
          std::pmr::monotonic_buffer_resource resource{buf.data(), buf.size()};
          std::pmr::polymorphic_allocator<std::sub_match<it_t>> alloc{
              &resource};
          std::pmr::match_results<it_t> match{alloc};

          constexpr std::string_view magic{"Content-Length"};
          size_t content_length = 0;
          if (std::regex_match(beg, crlf, match, header_re)) {
            if (std::equal(match[1].first, match[1].second, magic.begin())) {
              for (auto cp = match[2].first;
                   cp != match[2].second && static_cast<bool>(isdigit(*cp));
                   ++cp)
                content_length
                    = content_length * 10 + static_cast<size_t>(*cp - '0');
            }
          }
          _buf.consume(static_cast<size_t>(crlf - beg) + searcher.size());
        }

        // We're now officially reading the message body, but there
        // may be some of the message (or all of it) in _buf.
        stage = reading_body;
        auto sz = _buf.size();
        auto from = asio::buffers_begin(_buf.data());
        auto to
            = from + static_cast<std::ptrdiff_t>(std::min(_content_length, sz));
        std::copy(from, to, _msg_buf.data());
        _buf.consume(static_cast<size_t>(to - from));

        if (sz < _content_length) {
          stage = reading_body;
          asio::async_read(_in,
                           asio::buffer(&_msg_buf[sz], _content_length - sz),
                           std::move(self));
          return;
        }
        goto done; // NOLINT
      }
      case reading_body: {
        if (ec) {
          self.complete(ec, {});
          return;
        }
      done:
        self.complete(
            {}, json::parse(std::string_view{_msg_buf.data(), _msg_buf.size()})
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

namespace lsplex::jsonrpc {
template <typename Readable> template <typename Token>
[[nodiscard]] auto istream<Readable>::async_get(Token&& tok) {
  return asio::async_compose<Token, void(boost::system::error_code,
                                         boost::json::object)>(
      detail::read_op{_in, _buf}, tok, _in);
}
template <typename Writable> template <typename Token>
[[nodiscard]] auto ostream<Writable>::async_put(const json::object& o,
                                                Token&& tok) {
  return asio::async_compose<Token, void(boost::system::error_code)>(
      detail::write_op{_out, o, _out_buf}, tok, _out);
}
}  // namespace lsplex::jsonrpc
