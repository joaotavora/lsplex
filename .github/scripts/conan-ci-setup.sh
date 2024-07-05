#!/bin/bash
#
# Stolen and adapted from https://github.com/friendlyanon/cmake-init.
#
# Instead of this clever script, we could require CMake 2.24 which
# supports CMAKE_PROJECT_TOP_LEVEL_INCLUDES which would enable us to
# use https://github.com/conan-io/cmake-conan.
#
# Not only does that seem slightly more idiomatic but it would
# hopefully allow us to automatically CMake's CMAKE_BUILD_TYPE with
# our profile build type.  But CMake 2.24 is too new for GitHub CI's
# default runners, so we just have to remember to use ONLY Release in
# the CI.  Which, might not be a bad idea anyway.
#
# We'd probably solve the default profile hacking below.

PS4='\033[1;34m>>>\033[0m '

set -xeu

pip3 install conan

conan profile detect -f

std=17
profile="$(conan profile path default)"

mv "$profile" "${profile}.bak"
sed 's/^\(compiler\.cppstd=\).\{1,\}$/\1'"$std/" "${profile}.bak" > "$profile"
rm "${profile}.bak"

if [ -f conan_cache_save.tgz ]; then
  conan cache restore conan_cache_save.tgz
fi
conan remove \* --lru=1M -c
conan install . --settings build_type=Release --build=missing
conan cache save '*/*:*' --file=conan_cache_save.tgz

