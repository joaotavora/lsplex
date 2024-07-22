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
                   -DLsPlex_DEV=ON
configure-debug:                                       \
      CMAKE_FLAGS+=-DCMAKE_BUILD_TYPE=Debug            \
                   -DLsPlex_DEV=ON                    \
                   -DLsPlex_USE_SANITIZER=Address     \
                   -DLsPlex_USE_CCACHE=ON
configure-coverage:                                    \
      CMAKE_FLAGS+=-DCMAKE_BUILD_TYPE=Release          \
                    -DLsPlex_DEV=ON                   \
                    -DLsPlex_USE_COVERAGE=ON
configure-%: phony
	cmake $(CMAKE_FLAGS) -S. -B build/$*

build-%: configure-% phony
	cmake --build build/$*

check-%: build-% phony
	ctest --test-dir build/$* --output-on-failure ${CTEST_OPTIONS}

watch-%: phony
	find CMakeLists.txt src include test -type f | entr -r -s 'make check-$*'

coverage: check-coverage
	./build/coverage/lsplex
	./build/coverage/lsplex -l martian || true
	./build/coverage/lsplex -l en
	./build/coverage/lsplex -l de
	./build/coverage/lsplex -l es
	./build/coverage/lsplex -l fr
	./build/coverage/lsplex --help
	./build/coverage/lsplex --version
	lcov -c -o build/coverage/coverage.info -d build/coverage  \
             --include "${PWD}/*"
	genhtml --legend -q build/coverage/coverage.info -p ${PWD} \
                -o build/coverage/coverage_html

run-%: configure-% phony
	cmake --build build/$* --target LsPlex_exe
	build/$*/lsplex --version

clean: phony
	rm -rf build*

doc: phony
	cmake -DLsPlex_DOCS=SKIP_OTHER_TARGETS -S. -B build-docs
	cmake --build build-docs --target LsPlexDocs

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
