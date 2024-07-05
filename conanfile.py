from conan import ConanFile

class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan" # put conan cruft here

    def requirements(self):
        self.requires("zlib/1.2.11")
        # self.requires("fmt/10.2.1")
        # self.requires("boost/1.85.0")
        # self.requires("cxxopts/3.2.0")

    def build_requirements(self):
        self.test_requires("doctest/2.4.11")
