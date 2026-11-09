cmake_minimum_required(VERSION 3.5)
project(rto4_node)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)

find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rto4_core REQUIRED)
find_package(rto4_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)

# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)


include_directories(
  include/rto4_node
)

add_executable(
  rto4_node
  src/rto4_node.cpp
  src/AnalogInputArrayROS.cpp
  src/BumperROS.cpp
  src/CameraROS.cpp
  src/ComROS.cpp
  src/DigitalInputArrayROS.cpp
  src/DigitalOutputArrayROS.cpp
  src/DistanceSensorArrayROS.cpp
  src/EncoderInputROS.cpp
  src/GyroscopeROS.cpp
  src/LaserRangeFinderROS.cpp
  src/MotorArrayROS.cpp
  src/OdometryROS.cpp
  src/OmniDriveROS.cpp
  src/PowerManagementROS.cpp
  src/RTONode.cpp
)

target_link_libraries(
  rto4_node
  rto4_core::api2
)


ament_target_dependencies(rto4_node rclcpp rto4_msgs std_msgs geometry_msgs sensor_msgs tf2 tf2_ros tf2_geometry_msgs nav_msgs builtin_interfaces) 

add_executable(
  rto4_laserrangefinder_node
  src/rto4_laserrangefinder_node.cpp
  src/ComROS.cpp
  src/LaserRangeFinderROS.cpp
  src/RTOLaserRangeFinderNode.cpp
)

ament_target_dependencies(rto4_laserrangefinder_node rclcpp rto4_msgs std_msgs geometry_msgs sensor_msgs tf2 tf2_ros builtin_interfaces) 


target_link_libraries(
  rto4_laserrangefinder_node
  rto4_core::api2
)

add_executable(
  rto4_camera_node
  src/rto4_camera_node.cpp
  src/ComROS.cpp
  src/CameraROS.cpp
  src/RTOCameraNode.cpp
)

ament_target_dependencies(rto4_camera_node rclcpp rto4_msgs std_msgs geometry_msgs sensor_msgs tf2 tf2_ros builtin_interfaces) 

target_link_libraries(
  rto4_camera_node
  rto4_core::api2
)

add_executable(
  rto4_odometry_node
  src/rto4_odometry_node.cpp
  src/ComROS.cpp
  src/OdometryROS.cpp
  src/RTOOdometryNode.cpp
)

ament_target_dependencies(rto4_odometry_node rclcpp rto4_msgs std_msgs geometry_msgs sensor_msgs tf2 tf2_ros tf2_geometry_msgs nav_msgs builtin_interfaces)

target_link_libraries(
  rto4_odometry_node
  rto4_core::api2
)

add_executable(
  rto4_node_emb_streamline
  src/rto4_node_emb_streamline.cpp
  src/ComROS.cpp
  src/OmniDriveROS.cpp
  src/RTOEmbStreamlineNode.cpp
)

ament_target_dependencies(rto4_node_emb_streamline rclcpp rto4_msgs std_msgs geometry_msgs sensor_msgs tf2 tf2_ros tf2_geometry_msgs nav_msgs builtin_interfaces)

target_link_libraries(
  rto4_node_emb_streamline
  rto4_core::api2
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  # CPU performance benchmarks (no Robotino needed). Only compiled on amd64
  # since the arch guard above early-returns on arm. See test/test_rto4_perf.cpp.
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_rto4_perf
    test/test_rto4_perf.cpp
    src/ComROS.cpp
    src/OdometryROS.cpp
    src/OmniDriveROS.cpp
    src/RTOEmbStreamlineNode.cpp)
  if(TARGET test_rto4_perf)
    target_include_directories(test_rto4_perf PRIVATE include/rto4_node)
    ament_target_dependencies(test_rto4_perf rclcpp rto4_msgs std_msgs geometry_msgs sensor_msgs tf2 tf2_ros tf2_geometry_msgs nav_msgs builtin_interfaces)
    target_link_libraries(test_rto4_perf rto4_core::api2)
  endif()
endif()

install(TARGETS
  rto4_node
  rto4_camera_node
  rto4_laserrangefinder_node
  rto4_odometry_node
  rto4_node_emb_streamline
  DESTINATION lib/${PROJECT_NAME})

ament_package()

if(BUILD_TESTING)
  find_package(tf2_msgs REQUIRED)
  ament_add_gtest(test_rto4_adapters
    test/test_rto4_adapters.cpp
    src/AnalogInputArrayROS.cpp
    src/BumperROS.cpp
    src/CameraROS.cpp
    src/DigitalInputArrayROS.cpp
    src/DistanceSensorArrayROS.cpp
    src/EncoderInputROS.cpp
    src/GyroscopeROS.cpp
    src/MotorArrayROS.cpp
    src/OdometryROS.cpp
    src/OmniDriveROS.cpp
    src/PowerManagementROS.cpp
    TIMEOUT 60)
  if(TARGET test_rto4_adapters)
    target_include_directories(test_rto4_adapters PRIVATE include/rto4_node)
    ament_target_dependencies(test_rto4_adapters
      rclcpp rto4_msgs std_msgs geometry_msgs sensor_msgs tf2 tf2_ros
      tf2_geometry_msgs tf2_msgs nav_msgs builtin_interfaces)
    target_link_libraries(test_rto4_adapters rto4_core::api2)
  endif()
endif()
