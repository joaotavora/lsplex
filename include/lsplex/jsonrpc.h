#pragma once

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/json/object.hpp>
#include <memory>

#include "lsplex/export.hpp"

namespace lsplex::jsonrpc {

namespace json = boost::json;
namespace asio = boost::asio;

namespace detail {
  struct istream_impl;
}  // namespace detail

LSPLEX_EXPORT class istream {
  std::unique_ptr<detail::istream_impl> _pimpl;

public:
  LSPLEX_EXPORT istream(const istream &) = delete;
  istream(istream &&) = delete;
  istream &operator=(const istream &) = delete;
  istream &operator=(istream &&) = delete;
  explicit istream(asio::posix::stream_descriptor d);
  LSPLEX_EXPORT [[nodiscard]] json::object get();
  ~istream(); // see https://stackoverflow.com/questions/9020372/how-do-i-use-unique-ptr-for-pimpl
};
}  // namespace lsplex::jsonrpc
