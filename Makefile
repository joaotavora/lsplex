# Targets for non-devs
build: phony build-nodev
install: install-nodev

build-nodev: CMAKE_FLAGS+=-DCMAKE_BUILD_TYPE=Release
install-nodev: build-nodev
ifdef PREFIX
	cmake --install build/nodev --prefix=${PREFIX}
else
	cmake --install build/nodev
endif

# Targets for devs
configure-release:                                     \
      CMAKE_FLAGS+=-DCMAKE_BUILD_TYPE=Release          \
                   -DGreeter_DEV=ON
configure-debug:                                       \
      CMAKE_FLAGS+=-DCMAKE_BUILD_TYPE=Debug            \
                   -DGreeter_DEV=ON                    \
                   -DGreeter_USE_SANITIZER=Address     \
                   -DGreeter_USE_CCACHE=ON
configure-coverage:                                    \
      CMAKE_FLAGS+=-DCMAKE_BUILD_TYPE=Release          \
                    -DGreeter_DEV=ON                   \
                    -DGreeter_USE_COVERAGE=ON
configure-%: phony
	cmake $(CMAKE_FLAGS) -S. -B build/$*

build-%: configure-% phony
	cmake --build build/$*

check-%: build-% phony
	ctest --test-dir build/$* --output-on-failure ${CTEST_OPTIONS}

watch-%: phony
	find CMakeLists.txt src include test -type f | entr -r -s 'make check-$*'

coverage: check-coverage
	./build/coverage/greeter
	./build/coverage/greeter -l martian || true
	./build/coverage/greeter -l en
	./build/coverage/greeter -l de
	./build/coverage/greeter -l es
	./build/coverage/greeter -l fr
	./build/coverage/greeter --help
	./build/coverage/greeter --version
	lcov -c -o build/coverage/coverage.info -d build/coverage  \
             --include "${PWD}/*"
	genhtml --legend -q build/coverage/coverage.info -p ${PWD} \
                -o build/coverage/coverage_html

run-%: configure-% phony
	cmake --build build-$* --target GreeterExec
	build-$*/Greeter --version

clean: phony
	rm -rf build*

doc: phony
	cmake -DGreeter_DOCS=ON -S. -B build-docs
	cmake --build build-docs --target GreeterDocs

compile_commands.json: configure-debug
	ln -sf build-debug/compile_commands.json compile_commands.json

check-format: phony
	find src include test -type f | xargs clang-format --dry-run --Werror
	find cmake -type f -iname "*.cmake" | xargs cmake-format CMakeLists.txt

fix-format: phony
	find src include test -type f | xargs clang-format -i
	find cmake -type f -iname "*.cmake" | xargs cmake-format -i CMakeLists.txt

fix-codespell: phony


.PHONY: phony
