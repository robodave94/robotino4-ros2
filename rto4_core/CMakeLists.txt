cmake_minimum_required(VERSION 3.16)
project(rto4_core VERSION 0.1.0 LANGUAGES CXX)

option(RTO4_CORE_STANDALONE "Build without ROS/ament" OFF)
if(NOT RTO4_CORE_STANDALONE)
  find_package(ament_cmake REQUIRED)
endif()
find_package(Qt5 REQUIRED COMPONENTS Core Network Xml Gui)
find_package(Threads REQUIRED)
find_package(JPEG REQUIRED)

# Preserve the licensed SDK tree; its legacy build scripts are not executed.
file(GLOB_RECURSE rpc_sources CONFIGURE_DEPENDS vendor/rec/rpc/*.cpp)
file(GLOB_RECURSE rpc_headers CONFIGURE_DEPENDS vendor/rec/rpc/*.h vendor/rec/rpc/*.hpp)
add_library(rto4_core_rpc SHARED ${rpc_sources} ${rpc_headers})
add_library(rto4_core::rpc ALIAS rto4_core_rpc)
set_target_properties(rto4_core_rpc PROPERTIES
  AUTOMOC ON
  EXPORT_NAME rpc
  VERSION ${PROJECT_VERSION}
  SOVERSION 0)
target_compile_features(rto4_core_rpc PUBLIC cxx_std_14)
target_include_directories(rto4_core_rpc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(rto4_core_rpc PUBLIC Qt5::Core Qt5::Network Qt5::Xml Qt5::Gui
  PRIVATE Threads::Threads)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/rec/rpc)
configure_file(cmake/rec_rpc_version.h.in
  generated/rec/rpc/rec_rpc_version.h @ONLY)

file(GLOB_RECURSE robotino_rpc_sources CONFIGURE_DEPENDS
  vendor/rec/robotino/rpc/*.cpp)
file(GLOB_RECURSE robotino_rpc_headers CONFIGURE_DEPENDS
  vendor/rec/robotino/rpc/*.h vendor/rec/robotino/rpc/*.hpp)
add_library(rto4_core_robotino_rpc SHARED ${robotino_rpc_sources} ${robotino_rpc_headers})
add_library(rto4_core::robotino_rpc ALIAS rto4_core_robotino_rpc)
set_target_properties(rto4_core_robotino_rpc PROPERTIES
  AUTOMOC ON
  EXPORT_NAME robotino_rpc
  VERSION ${PROJECT_VERSION}
  SOVERSION 0)
target_compile_features(rto4_core_robotino_rpc PUBLIC cxx_std_14)
target_include_directories(rto4_core_robotino_rpc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(rto4_core_robotino_rpc PUBLIC
  rto4_core::rpc Qt5::Core Qt5::Network Qt5::Xml Qt5::Gui
  PRIVATE Threads::Threads)

file(GLOB_RECURSE fleetcom_sources CONFIGURE_DEPENDS
  vendor/rec/robotino3/fleetcom/*.cpp)
file(GLOB_RECURSE fleetcom_headers CONFIGURE_DEPENDS
  vendor/rec/robotino3/fleetcom/*.h vendor/rec/robotino3/fleetcom/*.hpp)
add_library(rto4_core_fleetcom SHARED ${fleetcom_sources} ${fleetcom_headers})
add_library(rto4_core::fleetcom ALIAS rto4_core_fleetcom)
set_target_properties(rto4_core_fleetcom PROPERTIES
  AUTOMOC ON
  EXPORT_NAME fleetcom
  VERSION ${PROJECT_VERSION}
  SOVERSION 0)
target_compile_features(rto4_core_fleetcom PUBLIC cxx_std_14)
target_include_directories(rto4_core_fleetcom PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(rto4_core_fleetcom PUBLIC Qt5::Core Qt5::Network)

file(GLOB cv_lt_sources CONFIGURE_DEPENDS
  vendor/rec/cv_lt/*.cpp)
file(GLOB cv_lt_headers CONFIGURE_DEPENDS
  vendor/rec/cv_lt/*.h vendor/rec/cv_lt/*.hpp
  vendor/rec/cv_lt/jpeg/*.h)
add_library(rto4_core_cv_lt SHARED ${cv_lt_sources} ${cv_lt_headers})
add_library(rto4_core::cv_lt ALIAS rto4_core_cv_lt)
set_target_properties(rto4_core_cv_lt PROPERTIES
  EXPORT_NAME cv_lt
  VERSION ${PROJECT_VERSION}
  SOVERSION 0)
target_compile_features(rto4_core_cv_lt PUBLIC cxx_std_14)
target_include_directories(rto4_core_cv_lt PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(rto4_core_cv_lt PUBLIC Qt5::Core JPEG::JPEG)

file(GLOB_RECURSE robotino_api2_sources CONFIGURE_DEPENDS
  vendor/rec/robotino/api2/*.cpp)
file(GLOB_RECURSE robotino_api2_headers CONFIGURE_DEPENDS
  vendor/rec/robotino/api2/*.h vendor/rec/robotino/api2/*.hpp vendor/rec/robotino/api2/*.hh)
add_library(rto4_core_api2 SHARED ${robotino_api2_sources} ${robotino_api2_headers})
add_library(rto4_core::api2 ALIAS rto4_core_api2)
set_target_properties(rto4_core_api2 PROPERTIES
  AUTOMOC ON
  EXPORT_NAME api2
  VERSION ${PROJECT_VERSION}
  SOVERSION 0)
target_compile_features(rto4_core_api2 PUBLIC cxx_std_14)
target_include_directories(rto4_core_api2 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(rto4_core_api2 PUBLIC
  rto4_core::rpc
  rto4_core::robotino_rpc
  rto4_core::fleetcom
  rto4_core::cv_lt
  Qt5::Core Qt5::Network Qt5::Xml Qt5::Gui
  PRIVATE Threads::Threads m dl rt)

include(CTest)
if(BUILD_TESTING)
  find_package(GTest REQUIRED)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_executable(test_rpc test/test_rpc.cpp)
  target_link_libraries(test_rpc PRIVATE rto4_core::rpc GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(test_rpc DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30)
  add_executable(rpc_peer test/rpc_peer.cpp)
  target_link_libraries(rpc_peer PRIVATE rto4_core::rpc)
  add_test(NAME test_rpc_peers COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_peers.py $<TARGET_FILE:rpc_peer>)
  set_tests_properties(test_rpc_peers PROPERTIES TIMEOUT 40)
  add_executable(test_api2_compile test/test_api2_compile.cpp)
  target_link_libraries(test_api2_compile PRIVATE rto4_core::api2 GTest::gtest_main)
  gtest_discover_tests(test_api2_compile DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 30)
endif()

install(TARGETS
  rto4_core_rpc
  rto4_core_robotino_rpc
  rto4_core_fleetcom
  rto4_core_cv_lt
  rto4_core_api2
  EXPORT rto4_coreTargets
  LIBRARY DESTINATION lib ARCHIVE DESTINATION lib RUNTIME DESTINATION bin)
install(DIRECTORY vendor/rec/ DESTINATION include/rec
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/rec/rpc/rec_rpc_version.h
  DESTINATION include/rec/rpc)
install(FILES vendor/rec/rpc/LICENSE.txt README.md DESTINATION share/${PROJECT_NAME})
install(EXPORT rto4_coreTargets NAMESPACE rto4_core::
  DESTINATION share/${PROJECT_NAME}/cmake)
install(FILES cmake/dependencies.cmake DESTINATION share/${PROJECT_NAME}/cmake)

if(RTO4_CORE_STANDALONE)
  include(CMakePackageConfigHelpers)
  configure_package_config_file(cmake/rto4_coreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/rto4_coreConfig.cmake
    INSTALL_DESTINATION share/${PROJECT_NAME}/cmake)
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/rto4_coreConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rto4_coreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/rto4_coreConfigVersion.cmake
    DESTINATION share/${PROJECT_NAME}/cmake)
else()
  ament_export_targets(rto4_coreTargets HAS_LIBRARY_TARGET NAMESPACE rto4_core::)
  ament_package(CONFIG_EXTRAS cmake/dependencies.cmake)
endif()
