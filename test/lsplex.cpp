#include <doctest/doctest.h>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/json/object.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include "lsplex/jsonrpc.h"

namespace bp = boost::process;
namespace asio = boost::asio;
namespace json = boost::json;

TEST_CASE("Get JSON objects from jsonrpc::istream") {
  asio::io_context ctx;
  auto pin = std::make_unique<boost::asio::posix::stream_descriptor>(
      ctx,
      // NOLINTNEXTLINE(*vararg, android*));
      ::open("resources/jsonrpc_1.txt", O_RDONLY));
  lsplex::jsonrpc::istream is(std::move(pin));
  CHECK(is.get() == json::object{{"hello", 42}});
  CHECK(is.get() == json::object{{"hello", 43}});
  CHECK(is.get() == json::object{{"hello", 44}});
  CHECK(is.get() == json::object{{"hello", 45}});
  CHECK(is.get() == json::object{{"hello", 46}});
  CHECK(is.get() == json::object{{"hello", 47}});
}

TEST_CASE("Superbasic") {
  // using namespace lsplex; // NOLINT
  // LsInvocation i{{"echo"}};
  // LsPlex plex{{i}};
  // plex.start();

  // const json::object obj = {{"greeting", "Hello"}, {"temperature", 42.42}};

  // json::object reply;
  // plex >> reply;
}

// Just an example for future use
TEST_CASE("Reverse program test cases") {
  auto run_reverse_program = [](const std::string& input) {
    bp::ipstream stdout_stream;
    bp::opstream stdin_stream;
    bp::ipstream stderr_stream;

    // Using the rev command
    bp::child c("rev", bp::std_out > stdout_stream,
                bp::std_in<stdin_stream, bp::std_err> stderr_stream);

    // Write input to the program
    stdin_stream << input << "\n";
    stdin_stream.flush();
    stdin_stream.pipe().close();  // Close the pipe to indicate end of input

    // Read output from the program
    std::string output;
    stdout_stream >> output;

    // Check for any error output (optional)
    std::string error_output;
    std::getline(stderr_stream, error_output);

    c.wait();  // Wait for the process to exit

    if (c.exit_code() != 0) {
      throw std::runtime_error("The reverse program failed: " + error_output);
    }

    return output;
  };

  CHECK(run_reverse_program("hello") == "olleh");
  CHECK(run_reverse_program("world") == "dlrow");
  CHECK(run_reverse_program("abcd") == "dcba");
  CHECK(run_reverse_program("1234") == "4321");
  CHECK(run_reverse_program("a") == "a");
  CHECK(run_reverse_program("") == "");  // Test for empty string
}
