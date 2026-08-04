cmake_minimum_required(VERSION 3.10)

project(dr02_pro)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-fPIC)
add_compile_options(-w)
add_compile_options(-Wno-psabi)

set(BUILD_PLATFORM "x86" CACHE STRING "Target platform (x86 or arm)")

if(BUILD_PLATFORM STREQUAL "arm")
  set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
  set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")
endif()

option(BUILD_SIM "Build with simulation support" OFF)
if(BUILD_SIM)
  add_definitions(-DUSE_SIMULATION)
endif()

option(BUILD_DR02_PRO_LOW_LEVEL "Build DR02 Pro low-level examples" ON)
option(BUILD_DR02_PRO_HIGH_LEVEL "Build DR02 Pro high-level examples" ON)
option(BUILD_DR02_PRO_MONITORING "Build DR02 Pro monitoring examples" ON)
option(BUILD_DR02_PRO_PERIPHERALS "Build DR02 Pro peripheral examples" ON)
option(BUILD_DR02_PRO_AUDIO "Build DR02 Pro audio examples" ON)
option(BUILD_DR02_PRO_STATE_MACHINE "Build DR02 Pro state machine" ON)

find_package(ament_cmake REQUIRED)
find_package(deep_robotics_common REQUIRED)
find_package(rclcpp REQUIRED)
find_package(drdds REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

if(BUILD_DR02_PRO_STATE_MACHINE)
  add_subdirectory(state_machine)
endif()

if(BUILD_DR02_PRO_LOW_LEVEL)
  add_subdirectory(low_level)
endif()

if(BUILD_DR02_PRO_HIGH_LEVEL)
  add_subdirectory(high_level)
endif()

if(BUILD_DR02_PRO_MONITORING)
  add_subdirectory(monitoring)
endif()

if(BUILD_DR02_PRO_PERIPHERALS)
  add_subdirectory(peripherals)
endif()

if(BUILD_DR02_PRO_AUDIO)
  add_subdirectory(audio)
endif()

ament_package()
