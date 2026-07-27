file(GLOB AUDIO_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(example_source ${AUDIO_SOURCES})
  get_filename_component(example_name ${example_source} NAME_WE)
  add_executable(${example_name} ${example_source})
  target_include_directories(${example_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  ament_target_dependencies(${example_name}
    rclcpp
    drdds
    std_msgs
  )
  install(TARGETS ${example_name}
    RUNTIME DESTINATION lib/${PROJECT_NAME}
  )
endforeach()
