#include "lsplex/lsplex.h"

#include <fmt/core.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/exception/exception.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/process/v2.hpp>
#include <boost/process/v2/environment.hpp>
#include <stdexcept>

#include "jsonrpc/jsonrpc.h"
#include "jsonrpc/pal/pal.h"

namespace asio = boost::asio;
namespace bp2 = boost::process::v2;
namespace fs = boost::filesystem;

namespace lsplex {

LsPlex::LsPlex(std::vector<LsContact> contacts)
    : _contacts(std::move(contacts)) {}

enum class direction { client2server, server2client };

template <direction d, typename Source, typename Sink>
asio::awaitable<void> transfer(Source& source, Sink& sink) {
  for (;;) {
    auto object = co_await source.async_get();
    co_await sink.async_put(object);
  }
}

asio::awaitable<void> doit(auto& our_stdin, auto& our_stdout, auto& child_stdin,
                           auto& child_stdout, auto& child_proc) {
  try {
    using namespace asio::experimental::awaitable_operators;  // NOLINT
    co_await (transfer<direction::client2server>(our_stdin, child_stdin)
            && transfer<direction::server2client>(child_stdout, our_stdout));
  // In theory, we should be able to wait also on the child process in
  // this && chain, but we cant' because per-op cancellation is _not_
  // supported on Windows for asio::windows::basic_object_handle
  // (according to Klemens Morgenstern).  So we close the processe's
  // stdin handle which should be enough to convince it to kill
  // itself.
  child_stdin.handle().close();
  auto ret = co_await child_proc.async_wait(boost::asio::use_awaitable);
  fmt::println("Process exited with '{}'", ret);
  } catch (std::exception& e) {
    fmt::println(stderr, "Exception: {}", e.what());
  }
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

  fs::path resolved{};

  if (fs::exists(contact.exe())) {
    fmt::println("Found '{}'", contact.exe());
    resolved = contact.exe();
  } else if (fs::path(contact.exe()).parent_path().empty()) {
    fmt::println("Looking for '{}' in PATH", contact.exe());
    resolved = bp2::environment::find_executable(contact.exe());
  }

  if (resolved.empty())
    throw std::runtime_error(fmt::format("Can't find '{}'", contact.exe()));

  jsonrpc::istream child_out{asio::readable_pipe{ioc}};
  jsonrpc::ostream child_in{asio::writable_pipe{ioc}};

  bp2::process proc{
      ioc, resolved, contact.args(),
      bp2::process_stdio{child_in.handle(), child_out.handle(), {}}};

  asio::co_spawn(ioc, doit(our_stdin, our_stdout, child_in, child_out, proc),
                 asio::detached);
  ioc.run();
}

}  // namespace lsplex
