#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, tools


class SqliteConan(ConanFile):
    name = "vitasqlite"
    version = "0.0.1"
    settings = "os", "compiler", "build_type", "arch"

    def source(self):
        source_url = "https://github.com/VitaSmith/libsqlite"
        commit = "7bf41a937d0358a1f0740950b30e8444ca8beea0"
        self.run("git clone %s" % source_url)
        self.run("git checkout %s" % commit, cwd="libsqlite")

        tools.download("https://www.sqlite.org/2018/sqlite-amalgamation-3230100.zip", "sqlite.zip")
        tools.unzip("sqlite.zip")

    def build(self):
        self.run("make -C libsqlite/libsqlite")

    def package(self):
        self.copy("*.h", src="libsqlite/libsqlite", dst="include/psp2", keep_path=False)
        self.copy("sqlite3.h", src="sqlite-amalgamation-3230100", dst="include", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["sqlite", "SceSqlite_stub"]
