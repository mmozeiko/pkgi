import os
from conans import ConanFile, CMake, tools, RunEnvironment


class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["FMT_HEADER_ONLY"] = self.options["fmt"].header_only
        cmake.configure()
        cmake.build()

    def test(self):
        pass
