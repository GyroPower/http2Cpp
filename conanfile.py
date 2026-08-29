from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class http2Cpp_recipe(ConanFile):
    name = "http2cpp"
    version = "0.0.1"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False], "h2load": [True, False]}
    default_options = {"shared": False, "fPIC": True, "h2load": False}

    exports_sources = "CMakeLists.txt", "http2Cpp/**", "logger/*"

    # generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("openssl/4.0.1", transitive_headers=True)
        self.requires("asio/1.38.0", transitive_headers=True)
        self.requires("libnghttp2/1.68.1", transitive_headers=True)
        self.requires("cli11/2.6.2", transitive_headers=True)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.h2load:
            self.options["libnghttp2/1.68.1"].with_app = True

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["libHttp2Cpp"]
        self.cpp_info.set_property("cmake_file_name", "libHttp2Cpp")
        self.cpp_info.set_property("cmake_target_name", "libHttp2Cpp::libHttp2Cpp")

    def layout(self):
        cmake_layout(self)
