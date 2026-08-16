from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMake, CMakeDeps, cmake_layout, CMakeToolchain


class server_client_recepie(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("http2cpp/0.0.1")

    def build(self):
        cmake = CMake(self)
        cmake.configure()

        copy(self, "compile_commands.json", self.build_folder, self.source_folder)

    def layout(self):
        cmake_layout(self)
