cmake_minimum_required(VERSION 2.8)
project(toxvpn)

set(BOOTSTRAP_PATH "bootstrap.json")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOTSTRAP_FILE=\\\"${BOOTSTRAP_PATH}\\\"")

if(WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32")
else(WIN32)
endif(WIN32)

if(STATIC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATIC")
set(LIBMODE STATIC)
endif(STATIC)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wconversion -Wshadow -Wextra -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(SYSTEMD)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSYSTEMD")
set(SYSTEMD_LIBRARIES "systemd")
endif(SYSTEMD)

if(ZMQ)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DZMQ")
set(ZMQ_LIBS "zmq")
endif(ZMQ)

find_library(SODIUM_LIBRARIES sodium REQUIRED "${SODIUM_PREFIX}/lib")
find_path(SODIUM_INCLUDE_DIRS sodium.h REQUIRED "${SODIUM_PREFIX}/include")

# Find Tox library using pkg-config
find_package(PkgConfig REQUIRED)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(TOX REQUIRED IMPORTED_TARGET libtoxcore)
endif()

if(WIN32)
  set(extra_files src/interface_windows.cpp src/route_windows.cpp)
elseif(CYGWIN)
  set(extra_files src/interface_windows.cpp src/route_windows.cpp)
elseif(APPLE)
  set(extra_files src/interface_mac.cpp src/route_mac.cpp)
else()
  set(extra_files src/interface_linux.cpp src/route_linux.cpp)
endif()

# Use the submodule libraries directly with relative paths
# First, we need to build the submodule
add_subdirectory(c-toxcore)

# Link against the toxcore library built from submodule (includes toxencryptsave)
set(TOXCORE_LIBRARIES toxcore_shared)

add_executable(toxvpn2 src/main.cpp src/control.cpp src/interface.cpp src/listener.cpp ${extra_files})
target_link_libraries(toxvpn2 pthread ${JSONCPP_LIBRARIES} ${SODIUM_LIBRARIES} ${TOXCORE_LIBRARIES})
include_directories(${JSONCPP_INCLUDE_DIRS} ${SODIUM_INCLUDE_DIRS} c-toxcore/toxcore)

add_executable(toxvpn-remote src/toxvpn-remote.cpp)
target_link_libraries(toxvpn-remote ${ZMQ_LIBS} ${SODIUM_LIBRARIES} ${TOXCORE_LIBRARIES})

if(WIN32)
  target_link_libraries(toxvpn2 ws2_32 ${SODIUM_LIBRARIES} pthread iphlpapi ${JSONCPP_LIBRARIES})
elseif(CYGWIN)
  target_link_libraries(toxvpn2 ws2_32 ${SODIUM_LIBRARIES} pthread iphlpapi)
elseif(APPLE)
  target_link_libraries(toxvpn2 ${ZMQ_LIBS})
else()
  if(STATIC)
    target_link_libraries(toxvpn2 cap ${ZMQ_LIBS} pthread ${SODIUM_LIBRARIES})
  else(STATIC)
    target_link_libraries(toxvpn2 cap ${ZMQ_LIBS} ${SYSTEMD_LIBRARIES})
  endif(STATIC)
endif()

install(TARGETS toxvpn2 toxvpn-remote DESTINATION bin)
install(FILES res/bootstrap.json DESTINATION share/toxvpn)
