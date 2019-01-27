#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools


class TaihenConan(ConanFile):
    name = "taihen"
    version = "0.11"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    exports_sources = ["CMakeLists.txt", "cmake.patch"]
    source_subfolder = "source_subfolder"
    build_subfolder = "build_subfolder"

    def source(self):
        source_url = "https://github.com/yifanlu/taiHEN"
        tag = "v0.11"
        self.run("git clone -b %s %s %s" % (tag, source_url, self.source_subfolder))
        self.run("git submodule update --init", cwd=self.source_subfolder)
        tools.patch(base_path=self.source_subfolder, patch_file="cmake.patch", strip=0)

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_folder=self.build_subfolder)
        cmake.build()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["taihen_stub"]
