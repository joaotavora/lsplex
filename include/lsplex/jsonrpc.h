#pragma once

#include <boost/asio/posix/basic_stream_descriptor.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/json/object.hpp>
#include <vector>

#include "lsplex/export.hpp"

namespace lsplex::jsonrpc {

LSPLEX_EXPORT class istream {
  using stream_t = boost::asio::posix::basic_stream_descriptor<>;
  std::unique_ptr<stream_t> _pin;
  boost::asio::streambuf _header_buf{30};
  std::vector<char> _msg_buf{};

public:
  LSPLEX_EXPORT explicit istream(std::unique_ptr<stream_t> pin) : _pin(std::move(pin)) {}
  LSPLEX_EXPORT [[nodiscard]] boost::json::object get();
};
}  // namespace lsplex::jsonrpc
