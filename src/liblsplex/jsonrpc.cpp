#include <lsplex/jsonrpc.h>

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/json/parse.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <memory_resource>
#include <regex>

namespace asio = boost::asio;

namespace lsplex::jsonrpc::detail {

struct parse_state {
  size_t content_length = 0;
  size_t erledigt = 0;
  bool at_eol{false};
  size_t consume{0};
};

struct parse_mandatory_headers {
  explicit parse_mandatory_headers(parse_state* pstate) : pstate(pstate) {}
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
        pstate->consume = eol_probe - begin;
        eol_probe++;
      }

      pstate->consume = eol_probe - begin;

      if (pstate->content_length != 0         // If got mandatory header
          && pstate->at_eol                   // and last saw a complete line
          && eol_probe - eol.size() == begin  // and looking at another CRLF...
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
            pstate->content_length = pstate->content_length * 10 + (*cp - '0');
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

namespace json = boost::json;
using boost::system::error_code;

[[nodiscard]] boost::json::object lsplex::jsonrpc::istream::get() {
  error_code ec;
  size_t read{};
  detail::parse_state state;
again:
  read = asio::read_until(*_pin, _header_buf,
                          detail::parse_mandatory_headers{&state}, ec);
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
  auto to = from + std::min(state.content_length, sz);  // NOLINT(*-narrowing-*)

  std::copy(from, to, _msg_buf.data());
  _header_buf.consume(to - from);

  if (sz < state.content_length)
    read = boost::asio::read(
        *_pin, asio::buffer(&_msg_buf[sz], state.content_length - sz));

  return json::parse(std::string_view{_msg_buf.data(), _msg_buf.size()})
      .as_object();
}

}  // namespace lsplex::jsonrpc
