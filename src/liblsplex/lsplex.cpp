#include <fmt/core.h>
#include <lsplex/jsonrpc.h>
#include <lsplex/lsplex.h>

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/json/serialize.hpp>

namespace asio = boost::asio;

namespace lsplex {

LsPlex::LsPlex(std::vector<LsInvocation> invocations)
    : _invocations(std::move(invocations)){};

void LsPlex::start() {
  (void)_invocations;
  asio::io_context ctx;
  // NOLINTNEXTLINE(android-cloexec-dup)
  jsonrpc::istream is{std::make_unique<asio::posix::stream_descriptor>(
      ctx, ::dup(STDIN_FILENO))};  // NOLINT

  auto a = is.get();
  fmt::println("Got this object {}", boost::json::serialize(a));
  // ctx.run();
}

}  // namespace lsplex
