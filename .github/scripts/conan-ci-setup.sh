#!/bin/bash
#
# Stolen and adapted from https://github.com/friendlyanon/cmake-init.
#
# Instead of this clever script, we could require CMake 2.24 which
# supports CMAKE_PROJECT_TOP_LEVEL_INCLUDES which would enable us to
# use https://github.com/conan-io/cmake-conan.  Could be we'd probably
# solve the default profile hacking below?

PS4='\033[1;34m>>>\033[0m '

set -xeuf

conan profile detect -f

profile="$(conan profile path default)"

# This is a bit ridiculous, but it works around sed discrepancies.
#
# Editing the guessed profile increases the chances to get much faster
# binary builds (i.e. build=missing is never triggered)
sed 's/^\(compiler\.cppstd=\).\{1,\}$/\1'"17/" "$profile" > "${profile}.1"
if grep -q '^compiler=apple-clang' "${profile}.1"; then
  sed 's/^\(compiler\.version=\).\{1,\}$/\113/' "${profile}.1" > "$profile"
else
  mv "${profile}.1" "$profile"
fi

conan remove -c * #  ensure local cache is pristine
conan install . --settings build_type=Release --build=missing

