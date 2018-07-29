#!/bin/bash

set -xe

rm -fr ci/build ci/buildhost

cd ci

./setup_conan.sh

export VITASDK=$(pipenv run conan info vitasdk-toolchain/06@blastrock/pkgj --paths -pr vita | grep -Po '(?<=package_folder: ).*$')
export CC=gcc-7
export CXX=g++-7

mkdir buildhost
cd buildhost
pipenv run conan install ../.. --build missing -s build_type=RelWithDebInfo -s compiler=gcc -s compiler.version=7 -s compiler.libcxx=libstdc++11
cmake -GNinja ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHOST_BUILD=ON -DPKGI_ENABLE_LOGGING=ON
ninja
cd ..

mkdir build
cd build
pipenv run conan install ../.. --build missing -pr vita -s build_type=RelWithDebInfo
cmake -GNinja ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja
cp pkgj pkgj.elf
cd ..
