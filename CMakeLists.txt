cmake_minimum_required(VERSION 3.6)

if(NOT HOST_BUILD)
  # must be done before project()
  if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
      set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
      message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
  endif()

  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
endif()

project(pkgj)

# useful for memory alignment constraints
set(CMAKE_CXX_STANDARD 17)

include(ExternalProject)

set(VITA_APP_NAME "PKGj")
set(VITA_TITLEID  "PKGJ00000")
set(VITA_VERSION  "0.15")

option(PKGI_ENABLE_LOGGING "enables debug logging over udp multicast" OFF)

if(PKGI_ENABLE_LOGGING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPKGI_ENABLE_LOGGING")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPKGI_ENABLE_LOGGING")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPKGI_VERSION=\\\"${VITA_VERSION}\\\" -D_GNU_SOURCE -g -Wall -Wextra -Werror -fvisibility=hidden")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPKGI_VERSION=\\\"${VITA_VERSION}\\\" -D_GNU_SOURCE -g -Wall -Wextra -fvisibility=hidden")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")

ExternalProject_Add(Boost
  URL https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/Boost-prefix/src/Boost)

ExternalProject_Add(fmtproject
  URL https://github.com/fmtlib/fmt/releases/download/4.1.0/fmt-4.1.0.zip
  CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "-DFMT_TEST=OFF" "-DHAVE_OPEN=OFF" "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/fmtinst"
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/fmtinst/lib/libfmt.a
)
add_library(fmt STATIC IMPORTED GLOBAL)
set_property(TARGET fmt PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/fmtinst/lib/libfmt.a)
#if we don't make the dir, cmake is unhappy
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmtinst/include)
#actually it doesn't work, vitasdk's gcc thinks that -isystem includes are C
#set_property(TARGET fmt PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/fmtinst/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/fmtinst/include)
add_dependencies(fmt fmtproject)

if(HOST_BUILD)
  include(host.cmake)
else()
  include(cross.cmake)
endif()
