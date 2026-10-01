cmake_minimum_required(VERSION 3.5)
project(rto4_bringup)

find_package(ament_cmake REQUIRED)

# Headless Pi build (PI_BUILD=1): skip installing RViz-only assets.
if(DEFINED ENV{PI_BUILD} AND NOT "$ENV{PI_BUILD}" STREQUAL "0")
  install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})
else()
  install(DIRECTORY launch rviz DESTINATION share/${PROJECT_NAME})
endif()

ament_package()
