get_filename_component(DEEP_ROBOTICS_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(DEEP_ROBOTICS_THIRD_PARTY "${DEEP_ROBOTICS_SDK_ROOT}/third_party")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/interface
  ${CMAKE_CURRENT_SOURCE_DIR}/run_policy
  ${CMAKE_CURRENT_SOURCE_DIR}/state
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${DEEP_ROBOTICS_THIRD_PARTY}/eigen
  ${DEEP_ROBOTICS_THIRD_PARTY}/onnxruntime/${BUILD_PLATFORM}/include
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_INSTALL_RPATH
  ${DEEP_ROBOTICS_THIRD_PARTY}/onnxruntime/${BUILD_PLATFORM}/lib/
)

add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES IMPORTED_LOCATION "${DEEP_ROBOTICS_THIRD_PARTY}/onnxruntime/${BUILD_PLATFORM}/lib/libonnxruntime.so")

add_executable(state_machine main.cpp)

ament_target_dependencies(state_machine
  deep_robotics_common
  rclcpp
  drdds
  sensor_msgs
  std_msgs
)

target_link_libraries(state_machine
  onnxruntime
  pthread
  m
  rt
  dl
)

install(TARGETS state_machine
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)
