# Example server

Here is just a test demo of the implementation for server in Http2Cpp, If you build and installed the library you can just run one of the two scripts or run yourself the commands in the terminal.

# Build 

Be aware of linking the same std backend if you compile with clang using libc++ or stdlibc++11 in the conan profile you provided or if you get the sources of the libraries by yourself and compile it with one of those std backends using clang.

If you get the sources by your package manager and get undefined reference errors, you will have to check or try with which runtime works, by default the scripts with conan with clang as compiler use libc++ from llvm. For help use -h. 

Run one of the scripts for one of the methods you install the library, only generates the files but it does not run build, you have to do it.

Conan. 
```python build-conan```

System installed libraries with cmake.
```python build-cmake-system```

