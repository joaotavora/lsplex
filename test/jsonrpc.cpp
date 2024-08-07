#include <doctest/doctest.h>
#include <jsonrpc/jsonrpc.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/file_base.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/json/object.hpp>
#include <boost/process/v2.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>
#include <cstdio>

#include "jsonrpc/pal/pal.h"

namespace bp2 = boost::process::v2;
namespace asio = boost::asio;
namespace json = boost::json;
namespace jsonrpc = lsplex::jsonrpc;

TEST_CASE("Get JSON objects from stdin") {
  asio::thread_pool ioc{1};

  jsonrpc::istream is{
      jsonrpc::pal::readable_file{ioc, "resources/jsonrpc_1.txt"}};
  CHECK(is.get() == json::object{{"hello", 42}});
  CHECK(is.get() == json::object{{"hello", 43}});
  CHECK(is.get() == json::object{{"hello", 44}});
  CHECK(is.get() == json::object{{"hello", 45}});
  CHECK(is.get() == json::object{{"hello", 46}});
  CHECK(is.get() == json::object{{"hello", 47}});
}

TEST_CASE("Get JSON objects from a process's stdout") {
  asio::thread_pool ioc{1};

  jsonrpc::istream is{asio::readable_pipe{ioc}};

  bp2::process proc{
      ioc,
#ifdef _MSC_VER
      bp2::environment::find_executable("more"),
#else
      bp2::environment::find_executable("cat"),
#endif
      {},
      bp2::process_stdio{
          boost::filesystem::path{"resources/jsonrpc_1.txt"}, is.handle(), {}}};

  CHECK(is.get() == json::object{{"hello", 42}});
  CHECK(is.get() == json::object{{"hello", 43}});
  CHECK(is.get() == json::object{{"hello", 44}});
  CHECK(is.get() == json::object{{"hello", 45}});
  CHECK(is.get() == json::object{{"hello", 46}});
  CHECK(is.get() == json::object{{"hello", 47}});
}
