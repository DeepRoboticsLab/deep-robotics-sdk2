file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(example_source ${EXAMPLE_SOURCES})
  get_filename_component(example_name ${example_source} NAME_WE)
  add_executable(${example_name} ${example_source})
  ament_target_dependencies(${example_name}
    rclcpp
    drdds
  )
  install(TARGETS ${example_name}
    RUNTIME DESTINATION lib/${PROJECT_NAME}
  )
endforeach()
