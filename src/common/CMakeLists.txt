cmake_minimum_required(VERSION 3.10)

project(deep_robotics_common)

if(DEFINED BUILD_PLATFORM)
  set(BUILD_PLATFORM "${BUILD_PLATFORM}" CACHE STRING "Target platform used by product packages")
endif()

find_package(ament_cmake REQUIRED)

get_filename_component(DEEP_ROBOTICS_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(DEEP_ROBOTICS_THIRD_PARTY "${DEEP_ROBOTICS_SDK_ROOT}/third_party")
set(EIGEN_INCLUDE_DIR "${DEEP_ROBOTICS_THIRD_PARTY}/eigen")

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${EIGEN_INCLUDE_DIR}>"
  "$<INSTALL_INTERFACE:include>"
  "$<INSTALL_INTERFACE:${EIGEN_INCLUDE_DIR}>"
)

install(DIRECTORY include/
  DESTINATION include
)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
)

ament_export_targets(export_${PROJECT_NAME})
ament_export_include_directories(include "${EIGEN_INCLUDE_DIR}")
ament_package()
