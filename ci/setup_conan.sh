#!/bin/bash

set -xe

pipenv install

# create home dirs
pipenv run conan

if ! [[ -e $HOME/.conan/profiles/vita ]] ; then
  mkdir -p $HOME/.conan/profiles
  cp -f conan/profiles/vita $HOME/.conan/profiles
fi

if [[ -e $HOME/.conan/settings.yml ]]; then
  if ! grep PSVita $HOME/.conan/settings.yml ; then
    echo "Your ~/.conan/settings.yml does not contain PSVita, you must update it manually with ci/conan/settings.yml"
    exit 1
  fi
else
  cp conan/settings.yml $HOME/.conan/settings.yml
fi

if ! pipenv run conan remote list | grep bincrafters; then
  pipenv run conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
fi

pipenv run conan export conan-vitasdk blastrock/pkgj
pipenv run conan export conan-fmt blastrock/pkgj
pipenv run conan export conan-bzip2 blastrock/pkgj
pipenv run conan export conan-libzip blastrock/pkgj
pipenv run conan export conan-vitasqlite blastrock/pkgj
pipenv run conan export conan-imgui blastrock/pkgj
pipenv run conan export conan-taihen blastrock/pkgj
