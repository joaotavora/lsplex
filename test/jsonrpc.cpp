#include <doctest/doctest.h>
#include <lsplex/jsonrpc.h>

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/json/object.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <boost/process/pipe.hpp>
#include <cstdio>

namespace bp = boost::process;
namespace asio = boost::asio;
namespace json = boost::json;
using asio::posix::stream_descriptor;
namespace jsonrpc = lsplex::jsonrpc;

TEST_CASE("Get JSON objects from stdin") {
  asio::thread_pool ioc{1};

  jsonrpc::istream is{stream_descriptor{
      ioc, ::open("resources/jsonrpc_1.txt", O_RDONLY | O_CLOEXEC)}};
  CHECK(is.get() == json::object{{"hello", 42}});
  CHECK(is.get() == json::object{{"hello", 43}});
  CHECK(is.get() == json::object{{"hello", 44}});
  CHECK(is.get() == json::object{{"hello", 45}});
  CHECK(is.get() == json::object{{"hello", 46}});
  CHECK(is.get() == json::object{{"hello", 47}});
}

TEST_CASE("Get JSON objects from a process's stdout") {
  asio::thread_pool ioc{1};

  bp::pipe child_out;
  bp::pipe child_in;
  // clang-format off
  bp::child c{"cat", bp::std_out > child_out,
                     bp::std_in  < child_in,
                     bp::std_err > stderr};
  // clang-format on

  child_out.assign_source(::open("resources/jsonrpc_1.txt", O_RDONLY | O_CLOEXEC));
  jsonrpc::istream is{stream_descriptor{ioc, child_out.native_source()}};

  CHECK(is.get() == json::object{{"hello", 42}});
  CHECK(is.get() == json::object{{"hello", 43}});
  CHECK(is.get() == json::object{{"hello", 44}});
  CHECK(is.get() == json::object{{"hello", 45}});
  CHECK(is.get() == json::object{{"hello", 46}});
  CHECK(is.get() == json::object{{"hello", 47}});
}
