cmake_minimum_required(VERSION 3.0)
project(harpoon2)
set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CURSES_NEED_NCURSES TRUE)

set(INCLUDES)
set(LIBRARIES)
set(DEPENDENCIES)

include(ExternalProject)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

if (NOT USE_WEBSOCKETPP_FROM_GIT)
  find_package(WebsocketPP REQUIRED)
endif()
if (NOT WEBSOCKETPP_FOUND)
  message("Using external project for websocketpp")
  ExternalProject_Add(websocketpp
          PREFIX "${PROJECT_BINARY_DIR}"
          GIT_REPOSITORY https://github.com/zaphoyd/websocketpp
          BUILD_IN_SOURCE ON
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          INSTALL_COMMAND "")
  ExternalProject_Get_Property(websocketpp SOURCE_DIR)
  list(APPEND INCLUDES ${SOURCE_DIR})
  list(APPEND DEPENDENCIES websocketpp)
endif()
if (USE_UTFCPP_FROM_GIT)
  ExternalProject_Add(utfcpp
          PREFIX "${PROJECT_BINARY_DIR}"
          GIT_REPOSITORY https://github.com/nemtrif/utfcpp
          BUILD_IN_SOURCE ON
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          INSTALL_COMMAND "")
  ExternalProject_Get_Property(utfcpp SOURCE_DIR)
  list(APPEND INCLUDES ${SOURCE_DIR}/source)
  list(APPEND DEPENDENCIES utfcpp)
else()
  find_package(UtfCpp REQUIRED)
endif()

find_package(Boost REQUIRED COMPONENTS system program_options date_time)
find_package(Curses REQUIRED)

pkg_search_module(JSONCPP REQUIRED jsoncpp)

file(GLOB_RECURSE SOURCES src/*.cpp)

list(APPEND INCLUDES
  ${OPENSSL_INCLUDE_DIR}
  ${WEBSOCKETPP_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  ${UTFCPP_INCLUDE_DIRS}
  ${CURSES_INCLUDE_DIR})
list(APPEND LIBRARIES
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENSSL_LIBRARIES}
  ${WEBSOCKETPP_LIBRARY}
  ${Boost_LIBRARIES}
  ${JSONCPP_LIBRARIES}
  ${CURSES_LIBRARIES})

add_executable(harpoon2 ${SOURCES})
target_include_directories(harpoon2 PUBLIC ${INCLUDES})
target_link_libraries(harpoon2 ${LIBRARIES})
if (DEPENDENCIES)
  add_dependencies(harpoon2 ${DEPENDENCIES})
endif()
if (USE_DEBUGLOG)
  target_compile_definitions(harpoon2 PUBLIC -DUSE_DEBUGLOG)
endif()

