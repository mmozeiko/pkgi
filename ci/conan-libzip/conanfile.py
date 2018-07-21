#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
import os


class LibzipConan(ConanFile):
    name = "libzip"
    version = "1.5.1"
    license = "BSD"
    exports_sources = ['CMakeLists.txt', 'forvita.patch', 'zlib-config.patch']
    generators = 'cmake'
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = "shared=False", "fPIC=False"
    source_subfolder = "source_subfolder"
    build_subfolder = "build_subfolder"
    requires = "zlib/1.2.11@conan/stable", "bzip2/1.0.6@blastrock/pkgj"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared and self.settings.os != "Windows":
            self.options.fPIC = True

    def source(self):
        source_url = "https://github.com/nih-at/libzip"
        tag = "rel-" + self.version.replace('.', '-')
        tools.get("{0}/archive/{1}.tar.gz".format(source_url, tag))
        extracted_dir = self.name + "-" + tag
        os.rename(extracted_dir, self.source_subfolder)

        tools.patch(patch_file="forvita.patch", base_path=self.source_subfolder)
        tools.patch(patch_file="zlib-config.patch", base_path=self.source_subfolder)

    def build(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_SHARED_LIBS"] = self.options.shared
        cmake.definitions["ENABLE_COMMONCRYPTO"] = False
        cmake.definitions["ENABLE_GNUTLS"] = False
        cmake.definitions["ENABLE_OPENSSL"] = False
        if self.settings.os != "Windows":
            cmake.definitions["CMAKE_POSITION_INDEPENDENT_CODE"] = self.options.fPIC
        cmake.configure(build_folder=self.build_subfolder)
        cmake.build()
        cmake.install()

    def package(self):
        self.copy("LICENSE", dst="license", src=self.source_subfolder, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["zip"]

        if self.options.shared:
            self.cpp_info.bindirs.append("lib")
