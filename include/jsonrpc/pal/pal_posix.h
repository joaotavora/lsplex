#pragma once

#include <boost/asio/posix/stream_descriptor.hpp>

namespace lsplex::jsonrpc::pal {

using boost::asio::posix::stream_descriptor;

struct readable_file : stream_descriptor {
  template <typename Executor>
  readable_file(Executor&& ex, const std::string& path)
    : stream_descriptor{std::forward<Executor>(ex), // FIXME: this
                                                    // doesn't really
                                                    // work
                          ::open(path.c_str(), O_RDONLY | O_CLOEXEC)} {}
};

struct asio_stdin : stream_descriptor {
  template <typename Executor> asio_stdin(Executor&& ex)
      : stream_descriptor{std::forward<Executor>(ex),
                          fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC)} {}
};

struct asio_stdout : stream_descriptor {
  template <typename Executor> asio_stdout(Executor&& ex)
      : stream_descriptor{std::forward<Executor>(ex),
                          fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC)} {}
};
}  // namespace lsplex::jsonrpc::pal
