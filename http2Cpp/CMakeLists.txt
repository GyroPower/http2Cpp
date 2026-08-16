#server
set(LIB_NAME http2Cpp)
file(GLOB_RECURSE SERVER_FILES
"${CMAKE_CURRENT_SOURCE_DIR}/src/http2Cpp/*.cpp"
${CMAKE_SOURCE_DIR}/logger.cpp)

add_library(${LIB_NAME} STATIC ${SERVER_FILES})

if(NOT CLI11_FOUND)

  message(STATUS "CLI11 make avaliable")

  include(FetchContent)

  FetchContent_Declare(CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG 2.6.2
  )

  FetchContent_MakeAvailable(CLI11)
else(CLI11_FOUND)
  message(STATUS "CLI11 found")

endif()

if(NOT asio_FOUND)
  find_path(ASIO_INCLUDE_DIR
    NAMES asio.hpp
    )

  # 2. Verify we found it
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Asio DEFAULT_MSG ASIO_INCLUDE_DIR)

  # 3. Create the target
  if(Asio_FOUND AND NOT TARGET asio::asio)
    add_library(asio::asio INTERFACE IMPORTED)
    set_target_properties(asio::asio PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ASIO_INCLUDE_DIR}"
    )
    # Standalone asio doesn't need Boost
    target_compile_definitions(asio::asio INTERFACE ASIO_STANDALONE)
    message("-- asio made")
  endif()

endif()

if(NOT libnghttp2_FOUND)
  message(STATUS "nghttp2 not found find_package")
  find_library(NGHTTP2_LIBRARY
        NAMES nghttp2 libnghttp2 nghttp2_static libnghttp2_static
        PATHS ${NGHTTP2_ROOT}/lib $ENV{NGHTTP2_ROOT}/lib
    )
  find_path(NGHTTP2_INCLUDE_DIR
        NAMES nghttp2/nghttp2.h
        PATHS ${NGHTTP2_ROOT}/include $ENV{NGHTTP2_ROOT}/include
    )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Nghttp2 DEFAULT_MSG
        NGHTTP2_LIBRARY NGHTTP2_INCLUDE_DIR)

  if(NGHTTP2_FOUND AND NOT TARGET nghttp2::nghttp2)
    add_library(libnghttp2::libnghttp2 UNKNOWN IMPORTED)
    set_target_properties(libnghttp2::libnghttp2 PROPERTIES
            IMPORTED_LOCATION "${NGHTTP2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${NGHTTP2_INCLUDE_DIR}"
        )
  endif()
endif()




if(LINUX AND LIBURING_FOUND)
  target_link_libraries(${LIB_NAME} PUBLIC
    ${LIBURING_LIBRARIES}
  )
endif()

if(USE_LIB_CXX)
  message(STATUS "libc++ using")

  target_compile_options(${LIB_NAME} PRIVATE $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>)
endif()


include(GNUInstallDirs)

if(NOT asio_FOUND)
  install(DIRECTORY ${ASIO_INCLUDE_DIR}/asio DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(DIRECTORY ${ASIO_INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "asio.hpp")
endif()


target_include_directories(${LIB_NAME}
PRIVATE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/http2Cpp/src/http2Cpp>"
"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/logger>"
PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")


install(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}Targets
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/"
DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/logger/"
DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")



install(EXPORT ${LIB_NAME}Targets
  FILE ${LIB_NAME}Targets.cmake
  NAMESPACE ${LIB_NAME}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "cmake/${LIB_NAME}Config.cmake.in"
  "${LIB_NAME}Config.cmake"
  INSTALL_DESTINATION
  "${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}"
)

write_basic_package_version_file(
  "${LIB_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}ConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}"
)

if(WIN32)
  target_link_options(${LIB_NAME} PRIVATE -static-libstdc++)

  add_compile_definitions(${LIB_NAME} PRIVATE _WIN32_WINNT=0x0A00)

  target_compile_definitions(${LIB_NAME} PRIVATE NGHTTP2_STATICLIB)

  target_link_libraries(${LIB_NAME} asio::asio Threads::Threads OpenSSL::SSL OpenSSL::Crypto ws2_32 mswsock libnghttp2::libnghttp2 CLI11::CLI11)

else()

  target_compile_definitions(${LIB_NAME} PUBLIC NGHTTP2_STATICLIB)

  target_link_libraries(${LIB_NAME} PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    asio::asio
    libnghttp2::libnghttp2
    CLI11::CLI11
  )

endif()
