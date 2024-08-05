#include "lsplex/lsplex.h"

#include <fmt/core.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/json/serialize.hpp>
#include <boost/process/v2.hpp>
#include <boost/process/v2/environment.hpp>
#include <stdexcept>

#include "jsonrpc/jsonrpc.h"
#include "jsonrpc/pal/pal.h"

namespace asio = boost::asio;
namespace bp = boost::process;
namespace bp2 = boost::process::v2;

namespace lsplex {

LsPlex::LsPlex(std::vector<LsContact> contacts)
    : _contacts(std::move(contacts)) {}

template <typename Source, typename Sink>
asio::awaitable<void> transfer(Source& source, Sink& sink) {
  for (;;) {
    auto object = co_await source.async_get();
    co_await sink.async_put(object);
  }
}

asio::awaitable<void> doit(auto& our_stdin,
                           auto& our_stdout,
                           auto& child_stdin,
                           auto& child_stdout,
                           auto& child_proc) {
  using namespace asio::experimental::awaitable_operators;  // NOLINT
  co_await (transfer(our_stdin, child_stdin)
            || transfer(child_stdout, our_stdout)
            || child_proc.async_wait(boost::asio::use_awaitable));
}

void LsPlex::start() {
  if (_contacts.size() == 0)
    throw std::runtime_error("Got to have some contacts!");

  if (_contacts.size() > 1)
    throw std::runtime_error("Currently support only one contact!!");

  asio::io_context ioc;
  auto contact = _contacts[0];

  jsonrpc::istream our_stdin{jsonrpc::pal::asio_stdin{ioc}};
  jsonrpc::ostream our_stdout{jsonrpc::pal::asio_stdout{ioc}};

  auto exe = bp2::environment::find_executable(contact.exe());
  if (exe.empty())
    throw std::runtime_error(fmt::format("Can't find {}", contact.exe()));

  jsonrpc::istream child_out{asio::readable_pipe{ioc}};
  jsonrpc::ostream child_in{asio::writable_pipe{ioc}};

  bp2::process proc{
      ioc, exe, contact.args(),
      bp2::process_stdio{child_in.handle(), child_out.handle(), {}}};

  asio::co_spawn(ioc, doit(our_stdin, our_stdout, child_in, child_out, proc),
                 asio::detached);
  ioc.run();
  fmt::println(stderr, "You shouldn't be seeing this");
}

}  // namespace lsplex
