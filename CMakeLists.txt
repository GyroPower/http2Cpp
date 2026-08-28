cmake_minimum_required(VERSION 4.2)

project(http2Cpp VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)


find_package(asio QUIET)
find_package(CLI11 2.6.2 QUIET)
find_package(OpenSSL 4 CONFIG REQUIRED COMPONENTS SSL Crypto)
find_package(Threads REQUIRED)
find_package(libnghttp2 QUIET)


if(LINUX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBURING REQUIRED liburing)
  add_compile_definitions(LINUX ASIO_HAS_IO_URING)
endif()

if("${CMAKE_BUILD_TYPE}" MATCHES "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported)

  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()

add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:RELEASE>
)

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID}")

# if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
#   message(STATUS "sanitizer on CLANG set to use")
#   add_compile_options(-O1 -g -fsanitize=address -fno-omit-frame-pointer )
#   add_link_options(-fsanitize=address)
# endif()



get_directory_property(_defs COMPILE_DEFINITIONS)

message(STATUS "All compile definitions: ${_defs}")
message(STATUS "Mode: ${CMAKE_BUILD_TYPE}")
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
#     add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
#     add_link_options(-fsanitize=address)
# endif()


if(INCLUDE_EXAMPLE)

  message(STATUS "Include example")
  add_subdirectory(example/server)
endif()

add_subdirectory(http2Cpp)




