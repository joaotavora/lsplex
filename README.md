[![Actions Status](https://github.com/joaotavora/lsplex/actions/workflows/ci.yml/badge.svg)](https://github.com/joaotavora/lsplex/actions)
[![codecov](https://codecov.io/gh/joaotavora/lsplex/branch/master/graph/badge.svg)](https://codecov.io/gh/joaotavora/lsplex)

# LsPlex

*Experimental/toy* LSP server proxy-multiplexer to solve
[these][eglot_issue1] [longstanding][eglot_issue3] [Eglot][eglot]
issues (now tracked
[here](https://github.com/joaotavora/eglot/discussions/1429)).

And perhaps other issues of other clients?

## Getting Started 

_This doesn't really do anything useful yet :-)_

If you're brave, grab 

* The C++ libraries `cxxopts`, `boost`, `fmt`
* CMake
* A C++ compiler toolchain

Then

```
make
```

If that (miraculously) succeeds, this invocation of the popular C++
language server `clangd` should now go through the proxy.

```
build/nodev/lsplex -- clangd --optional=arguments --to=clangd
```

## License

MIT

[eglot_issue1]: https://github.com/joaotavora/eglot/issues/249
[eglot_issue3]: https://github.com/joaotavora/eglot/issues/90
[eglot]: https://github.com/joaotavora/eglot
