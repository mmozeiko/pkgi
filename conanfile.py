from conans import ConanFile


class PkgjConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def requirements(self):
        if self.settings.os == "PSVita":
            self.requires("vitasqlite/0.0.1@blastrock/pkgj")
            self.requires("imgui/1.62@blastrock/pkgj")
        else:
            self.requires("sqlite3/3.21.0@bincrafters/stable")
            self.requires("boost_algorithm/1.66.0@bincrafters/stable")

        self.requires("libzip/1.5.1@blastrock/pkgj")
        self.requires("fmt/4.1.0@blastrock/pkgj")
        self.requires("boost_scope_exit/1.66.0@bincrafters/stable")
        self.requires("cereal/1.2.2@conan/stable")

    def configure(self):
        self.options["fmt"].fPIC = False
        self.options["fmt"].shared = False
        self.options["zlib"].shared = False
        self.options["bzip2"].shared = False
        self.options["bzip2"].fPIC = False
        self.options["libzip"].fPIC = False
        self.options["libzip"].shared = False
        self.options["imgui"].fPIC = False
        self.options["imgui"].shared = False
