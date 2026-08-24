file(GLOB HIGH_LEVEL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(example_source ${HIGH_LEVEL_SOURCES})
  get_filename_component(example_name ${example_source} NAME_WE)
  add_executable(${example_name} ${example_source})
  ament_target_dependencies(${example_name}
    deep_robotics_common
    rclcpp
    drdds
    std_msgs
  )
  target_include_directories(${example_name} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party
  )
  install(TARGETS ${example_name}
    RUNTIME DESTINATION lib/${PROJECT_NAME}
  )
endforeach()
