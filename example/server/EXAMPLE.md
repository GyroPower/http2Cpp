# Example server

Here is just a test demo of the implementation for server in Http2Cpp using CLI11 for command line arguments, If you build and installed the library you can just run one of the two scripts or run yourself the commands in the terminal, the example by default is included in the develop scripts for entry point and extend a real usage of the library.

# Build 

Be aware of linking the same std backend if you compile with clang using libc++ or stdlibc++11 in the conan profile you provided or if you get the sources of the libraries by yourself and compile it with one of those std backends using clang.

If you get the sources by your package manager and get undefined reference errors, you will have to check or try with which runtime works, by default the scripts with conan with clang as compiler use libc++ from llvm. For help use -h. 

Run one of the scripts for one of the methods you install the library, only generates the files but it does not run build, you have to do it.

Conan. 
```python build-conan```

System installed libraries with cmake.
```python build-cmake-system```

# Notes on Windows

There is a script for specific build process of the example server when it's installed the library with cmake, run -h to see arguments, it contemplates to use both official compilers from Visual Studio for c/c++, verify clang does not conflict with another clang instance on the path like clang from clang64 msys2
```python build-cmake-msvc-system.py -c=clang```

NOTE 
Still lack of script for automatize build for conan installation for Visual Studio build or to use directly msbuild
