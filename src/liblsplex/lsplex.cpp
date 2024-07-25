#include <fmt/core.h>
#include <lsplex/jsonrpc.h>
#include <lsplex/lsplex.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/json/serialize.hpp>
#include <boost/process.hpp>
#include <boost/process/io.hpp>
#include <boost/process/search_path.hpp>
#include <stdexcept>

namespace asio = boost::asio;
namespace posix = asio::posix;
namespace bp = boost::process;

namespace lsplex {

LsPlex::LsPlex(std::vector<LsContact> contacts)
    : _contacts(std::move(contacts)) {};

asio::awaitable<void> transfer(jsonrpc::istream& source,
                               jsonrpc::ostream& sink) {
  for (;;) {
    auto object = co_await source.async_get();
    co_await sink.async_put(object);
  }
}

asio::awaitable<void> doit(jsonrpc::istream& our_stdin,
                           jsonrpc::ostream& our_stdout,
                           jsonrpc::ostream& child_stdin,
                           jsonrpc::istream& child_stdout) {
  using namespace asio::experimental::awaitable_operators;  // NOLINT
  co_await (transfer(our_stdin, child_stdin)
            || transfer(child_stdout, our_stdout));
}

void LsPlex::start() {
  if (_contacts.size() == 0)
    throw std::runtime_error("Got to have some contacts!");

  if (_contacts.size() > 1)
    throw std::runtime_error("Currently support only one contact!!");

  asio::io_context ctx;
  auto contact = _contacts[0];

  jsonrpc::istream our_stdin{
      posix::stream_descriptor(ctx, fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC))};
  jsonrpc::ostream our_stdout{
      posix::stream_descriptor(ctx, fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC))};

  bp::pipe child_out_pipe;
  bp::pipe child_in_pipe;
  auto exe = bp::search_path(contact.exe());
  if (exe.empty())
    throw std::runtime_error(fmt::format("Can't find {}", contact.exe()));

  bp::child c{
      exe, contact.args(), bp::std_out > child_out_pipe,
      bp::std_in < child_in_pipe  // , bp::std_err> stderr
  };

  jsonrpc::ostream child_stdin{
      posix::stream_descriptor{ctx, child_in_pipe.native_sink()}};
  jsonrpc::istream child_stdout{
      posix::stream_descriptor{ctx, child_out_pipe.native_source()}};

  asio::co_spawn(ctx, doit(our_stdin, our_stdout, child_stdin, child_stdout),
                 asio::detached);
  ctx.run();
  fmt::println(stderr, "You shouldn't be seeing this");
}

}  // namespace lsplex
