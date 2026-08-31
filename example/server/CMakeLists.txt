cmake_minimum_required(VERSION 4.2)

project(http2CppExample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)


if(NOT INCLUDE_EXAMPLE)
  find_package(libHttp2Cpp QUIET)
endif()



set(CMAKE_EXPORT_COMPILE_COMMANDS true)

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

message(STATUS "Mode: ${CMAKE_BUILD_TYPE}")

set(EXAMPLE_EXE http2_server_byPower)


add_executable(${EXAMPLE_EXE} main.cpp)



# if(USE_LIB_CXX)
#   message(STATUS "libc++ using")
#   target_compile_options(${EXAMPLE_EXE} PRIVATE $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>)
# endif()

if(INCLUDE_EXAMPLE)
  message(STATUS "Include example")
  target_link_libraries(${EXAMPLE_EXE} PRIVATE
  libHttp2Cpp)

  target_include_directories(${EXAMPLE_EXE} PRIVATE "${CMAKE_SOURCE_DIR}/http2Cpp/src" "${CMAKE_SOURCE_DIR}/logger")

else()

  target_link_libraries(${EXAMPLE_EXE} PUBLIC
    libHttp2Cpp::libHttp2Cpp
  )
endif()

# if(LINUX)
#   target_link_libraries(${EXAMPLE_EXE} PRIVATE
#     ${LIBURING_LIBRARIES}
#   )
#
# endif()
