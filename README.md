
# implementation of HTTP2 on Asio (still lack of client library implementation)  

This is a simple example and learning project to do a very basic "framework" of a HTTP2 server using stand alone Asio, nghttp2 and OpenSSL.

## Dependecies 

- Python 3.14
- Clang or Gcc (MSVC not implemented yet)
- clangd lsp (for code navigation) Optional
- C++ 23 (minimum)
- Asio (1.38.2)
- OpenSSL (3.6.3)
- nghttp2 (1.68.1)
- Conan 2 (C/C++ package Manager) Optional  
- CMake 4.2 (minimum) 
- Ninja or Make for build (others tools not tested and implemented)


## Build and install
(Windows is not tested yet)
The preferred and the one I use is conan but if you had installed the libraries manually it will work too 

Clone the repo: 
```bash
git clone https://github.com/GyroPower/Http2Cpp.git
```

All build scripts receive arguments, use `-h` or `--help` for more info for arguments, all have default arguments.

To build and install the library using conan just run the script to run build-install-conan from the root to build by default debug clang linked to libc++, use one of the conan profiles if you want from conan-profiles or make your own conan profile or use one from the profile directory of your conan install, normally on your user directory called .conan2/profiles
```bash
python scripts/build-install-conan.py --profile=conan-profiles/my_conan_profile
```

Or if you make one on .conan2/profiles
```bash
python scripts/build-install-conan.py --profile=my_conan_profile
```


The build-install-cmake-system offer four arguments,use `-h` to get info of the arguments, to avoid run on sudo or as admin, is recommended to have defined the CMAKE_PREFIX_PATH env variable
```bash
python scripts/build-install-cmake-system.py -c=clang -bt=debug -bs=make --cmake-prefix-path=$HOME/.local/cmake
```


## Development and local testing

This section explains that those python scripts with "develop" is for just that, generates build files but does not build the library and it includes the example directory, just for toying or local development and it can be installed with the "install" scripts in the system.

build-conan-develop is for building locally the library, it only generates the build files but does not build, use the -h flag to get info for the scripts arguments

```bash
python scripts/build-conan-develop.py
```

build-cmake-system-develop is the same as above

test_server.py is for bench mark, run it and modify it if you need for select port and url request 


## Example 

There is a very minimal example code for the server and client api usage (not yet implemented client on the library), read the [EXAMPLE.md](example/server/EXAMPLE.md) on the example directory of server or client (not yet done the example for client)

Run the app using the .vscode directory launch.json for vscode or neovim 
(It assumes it's installed lldb extension for vscode or codelldb for neovim with DAP) or 
run it directly from the build directory in the terminal:
- From the project root directory 
```bash
./build/Debug/example/server/http2_server_byPower
```



