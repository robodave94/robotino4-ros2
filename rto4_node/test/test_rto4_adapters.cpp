// Copyright 2026 Robotino ROS 2 contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

#include "AnalogInputArrayROS.h"
#include "BumperROS.h"
#include "CameraROS.h"
#include "DigitalInputArrayROS.h"
#include "DistanceSensorArrayROS.h"
#include "EncoderInputROS.h"
#include "GyroscopeROS.h"
#include "JointStateMath.h"
#include "MotorArrayROS.h"
#include "OdometryROS.h"
#include "OmniDriveROS.h"
#include "PowerManagementROS.h"
#include "rec/robotino/api2/Com.h"
#include "tf2_msgs/msg/tf_message.hpp"

namespace
{
using namespace std::chrono_literals;
namespace api = rec::robotino::api2;

class AdapterTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    rclcpp::init(0, nullptr);
  }

  static void TearDownTestSuite()
  {
    rclcpp::shutdown();
  }

  void SetUp() override
  {
    node = std::make_shared<rclcpp::Node>("adapter_baseline");
    executor.add_node(node);
  }

  void TearDown() override
  {
    executor.remove_node(node);
    node.reset();
  }

  bool waitUntil(const std::function<bool()> & predicate)
  {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    do {
      executor.spin_once(10ms);
      if (predicate()) {
        return true;
      }
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
  }

  void drain()
  {
    const auto deadline = std::chrono::steady_clock::now() + 100ms;
    do {
      executor.spin_once(10ms);
    } while (std::chrono::steady_clock::now() < deadline);
  }

  template<typename Message>
  struct Capture
  {
    typename rclcpp::Subscription<Message>::SharedPtr subscription;
    std::shared_ptr<Message> message;
    size_t count = 0;
  };

  template<typename Message>
  std::shared_ptr<Capture<Message>> subscribe(const std::string & topic)
  {
    auto capture = std::make_shared<Capture<Message>>();
    std::weak_ptr<Capture<Message>> weak = capture;
    capture->subscription = node->create_subscription<Message>(
      topic, 10, [weak](std::shared_ptr<Message> message) {
        if (auto target = weak.lock()) {
          target->message = message;
          ++target->count;
        }
      });
    EXPECT_TRUE(waitUntil([capture]() {
        return capture->subscription->get_publisher_count() != 0;
    }));
    drain();
    return capture;
  }

  template<typename Message>
  bool received(const std::shared_ptr<Capture<Message>> & capture, size_t after = 0)
  {
    return waitUntil([capture, after]() {return capture->count > after;});
  }

  rclcpp::Node::SharedPtr node;
  rclcpp::executors::SingleThreadedExecutor executor;
  const rclcpp::Time stamp{12, 345678901, RCL_SYSTEM_TIME};
  const builtin_interfaces::msg::Time stamp_message = stamp;
};

TEST_F(AdapterTest, ValidVelocityClampPreservesZeroSignAndConfiguredBands)
{
  OmniDriveROS drive(node.get());
  drive.setMaxMin(2.3, 0.02, 1.0, 0.1);
  struct Case
  {
    double input;
    double linear;
    double angular;
  };
  const Case cases[] = {
    {0.0, 0.0, 0.0}, {0.001, 0.02, 0.1}, {-0.001, -0.02, -0.1},
    {0.02, 0.02, 0.1}, {-0.1, -0.1, -0.1}, {0.5, 0.5, 0.5},
    {1.0, 1.0, 1.0}, {2.3, 2.3, 1.0}, {20.0, 2.3, 1.0},
    {-20.0, -2.3, -1.0}};
  for (const auto & test : cases) {
    double x = test.input;
    double y = -test.input;
    double yaw = test.input;
    drive.clampVelocities(x, y, yaw);
    EXPECT_DOUBLE_EQ(x, test.linear);
    EXPECT_DOUBLE_EQ(y, -test.linear);
    EXPECT_DOUBLE_EQ(yaw, test.angular);
  }
  double x = -0.0;
  double y = 0.0;
  double yaw = -0.0;
  drive.clampVelocities(x, y, yaw);
  EXPECT_TRUE(std::signbit(x));
  EXPECT_TRUE(std::signbit(yaw));
  EXPECT_FALSE(std::signbit(y));
}

TEST_F(AdapterTest, AnalogCallbackPreservesStampOrderingAndShrinks)
{
  AnalogInputArrayROS adapter(node.get());
  adapter.setTimeStamp(stamp);
  auto capture = subscribe<rto4_msgs::msg::AnalogReadings>("analog_readings");
  const float values[] = {-1.5f, 0.0f, 2.75f};
  auto & callback = static_cast<api::AnalogInputArray &>(adapter);
  callback.valuesChangedEvent(values, 3);
  ASSERT_TRUE(received(capture));
  EXPECT_EQ(capture->message->stamp, stamp_message);
  EXPECT_EQ(capture->message->values, (std::vector<float>{-1.5f, 0.0f, 2.75f}));
  callback.valuesChangedEvent(values, 1);
  ASSERT_TRUE(received(capture, 1));
  EXPECT_EQ(capture->message->values, (std::vector<float>{-1.5f}));
  callback.valuesChangedEvent(nullptr, 0);
  drain();
  EXPECT_EQ(capture->count, 2u);
}

TEST_F(AdapterTest, DigitalCallbackConvertsAllNonzeroValuesToTrue)
{
  DigitalInputArrayROS adapter(node.get());
  adapter.setTimeStamp(stamp);
  auto capture = subscribe<rto4_msgs::msg::DigitalReadings>("digital_readings");
  const int values[] = {0, 1, -1, 42};
  auto & callback = static_cast<api::DigitalInputArray &>(adapter);
  callback.valuesChangedEvent(values, 4);
  ASSERT_TRUE(received(capture));
  EXPECT_EQ(capture->message->stamp, stamp_message);
  ASSERT_EQ(capture->message->values.size(), 4u);
  EXPECT_FALSE(capture->message->values[0]);
  for (size_t i = 1; i < 4; ++i) {
    EXPECT_TRUE(capture->message->values[i]);
  }
  callback.valuesChangedEvent(nullptr, 0);
  drain();
  EXPECT_EQ(capture->count, 1u);
}

TEST_F(AdapterTest, MotorVendorPositionCallbackDispatchesAndPublishesOnCurrent)
{
  MotorArrayROS adapter(node.get());
  adapter.setTimeStamp(stamp);
  auto capture = subscribe<rto4_msgs::msg::MotorReadings>("motor_readings");
  const float velocities[] = {-60.0f, 0.0f, 120.0f};
  const int positions[] = {-123456, 0, 654321};
  const float currents[] = {0.25f, 0.5f, 0.75f};
  auto & callback = static_cast<api::MotorArray &>(adapter);
  callback.velocitiesChangedEvent(velocities, 3);
  callback.positionsChangedEvent(positions, 3);
  drain();
  EXPECT_EQ(capture->count, 0u);
  std::vector<float> actual_velocities;
  std::vector<int> actual_positions;
  adapter.getMotorReadings(actual_velocities, actual_positions);
  EXPECT_EQ(actual_velocities, (std::vector<float>{-60.0f, 0.0f, 120.0f}));
  EXPECT_EQ(actual_positions, (std::vector<int>{-123456, 0, 654321}));
  callback.currentsChangedEvent(currents, 3);
  ASSERT_TRUE(received(capture));
  EXPECT_EQ(capture->message->stamp, stamp_message);
  EXPECT_EQ(capture->message->positions, actual_positions);
  EXPECT_EQ(capture->message->velocities, actual_velocities);
  EXPECT_EQ(capture->message->currents, (std::vector<float>{0.25f, 0.5f, 0.75f}));
}

TEST_F(AdapterTest, DistanceCallbackUsesHistoricalGeometryAndFramePrefix)
{
  DistanceSensorArrayROS adapter(node.get(), "robot/");
  adapter.setTimeStamp(stamp);
  auto capture = subscribe<sensor_msgs::msg::PointCloud>("distance_sensors");
  const float distances[] = {0.1f, 0.25f, 1.0f};
  auto & callback = static_cast<api::DistanceSensorArray &>(adapter);
  callback.distancesChangedEvent(distances, 3);
  ASSERT_TRUE(received(capture));
  EXPECT_EQ(capture->message->header.stamp, stamp_message);
  EXPECT_EQ(capture->message->header.frame_id, "robot/base_link");
  EXPECT_TRUE(capture->message->channels.empty());
  ASSERT_EQ(capture->message->points.size(), 3u);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_NEAR(capture->message->points[i].x, (distances[i] + 0.2) * std::cos(0.698 * i), 1e-6);
    EXPECT_NEAR(capture->message->points[i].y, (distances[i] + 0.2) * std::sin(0.698 * i), 1e-6);
    EXPECT_FLOAT_EQ(capture->message->points[i].z, 0.05f);
  }

  adapter.setFramePrefix("");
  callback.distancesChangedEvent(nullptr, 0);
  ASSERT_TRUE(received(capture, 1));
  EXPECT_TRUE(capture->message->points.empty());
  EXPECT_EQ(capture->message->header.frame_id, "base_link");
}

TEST_F(AdapterTest, JointStateStartupWaitsForBothCompleteMotorArrays)
{
  MotorArrayROS adapter(node.get());
  std::vector<float> velocities;
  std::vector<int> positions;
  std::vector<double> wheel_velocities{9.0, 8.0, 7.0};
  std::vector<double> wheel_positions{6.0, 5.0, 4.0};
  adapter.getMotorReadings(velocities, positions);
  EXPECT_TRUE(velocities.empty());
  EXPECT_TRUE(positions.empty());
  EXPECT_FALSE(rto4::populateWheelJointState(
    velocities, positions, wheel_velocities, wheel_positions));
  const float values[] = {60.0f, 120.0f, -180.0f};
  static_cast<api::MotorArray &>(adapter).velocitiesChangedEvent(values, 3);
  adapter.getMotorReadings(velocities, positions);
  EXPECT_FALSE(rto4::populateWheelJointState(
    velocities, positions, wheel_velocities, wheel_positions));
  const int ticks[] = {16, 32, -48};
  static_cast<api::MotorArray &>(adapter).positionsChangedEvent(ticks, 2);
  adapter.getMotorReadings(velocities, positions);
  EXPECT_FALSE(rto4::populateWheelJointState(
    velocities, positions, wheel_velocities, wheel_positions));
  EXPECT_EQ(wheel_velocities, (std::vector<double>{9.0, 8.0, 7.0}));
  EXPECT_EQ(wheel_positions, (std::vector<double>{6.0, 5.0, 4.0}));
  static_cast<api::MotorArray &>(adapter).positionsChangedEvent(ticks, 3);
  static_cast<api::MotorArray &>(adapter).velocitiesChangedEvent(values, 2);
  adapter.getMotorReadings(velocities, positions);
  EXPECT_FALSE(rto4::populateWheelJointState(
    velocities, positions, wheel_velocities, wheel_positions));
}

TEST_F(AdapterTest, JointStateKeepsWheelOrderingUnitsAndIgnoresFourthMotor)
{
  std::vector<double> velocities;
  std::vector<double> positions;
  ASSERT_TRUE(rto4::populateWheelJointState(
      {60.0f, 120.0f, -180.0f, 999.0f}, {16, 32, -48, 999},
    velocities, positions));
  ASSERT_EQ(velocities.size(), 3u);
  ASSERT_EQ(positions.size(), 3u);
  EXPECT_DOUBLE_EQ(velocities[0], -180.0 / 16 * (2 * 3.142) / 60);
  EXPECT_DOUBLE_EQ(velocities[1], 60.0 / 16 * (2 * 3.142) / 60);
  EXPECT_DOUBLE_EQ(velocities[2], 120.0 / 16 * (2 * 3.142) / 60);
  EXPECT_DOUBLE_EQ(positions[0], -3.0 * (2 * 3.142));
  EXPECT_DOUBLE_EQ(positions[1], 2 * 3.142);
  EXPECT_DOUBLE_EQ(positions[2], 2.0 * (2 * 3.142));
}

TEST_F(AdapterTest, EncoderCallbackCharacterizesUnsignedSchemaAndServiceName)
{
  EncoderInputROS adapter(node.get());
  adapter.setTimeStamp(stamp);
  auto capture = subscribe<rto4_msgs::msg::EncoderReadings>("encoder_readings");
  static_cast<api::EncoderInput &>(adapter).readingsChangedEvent(-60, -12345, 1.25f);
  ASSERT_TRUE(received(capture));
  EXPECT_EQ(capture->message->stamp, stamp_message);
  // Historical schema defects: signed ticks wrap and fractional amps truncate.
  EXPECT_EQ(capture->message->velocity, static_cast<uint32_t>(-60));
  EXPECT_EQ(capture->message->position, static_cast<uint32_t>(-12345));
  EXPECT_EQ(capture->message->current, 1u);
  const auto services = node->get_service_names_and_types();
  ASSERT_NE(services.find("/set_encoder_position"), services.end());
  EXPECT_EQ(services.at("/set_encoder_position"),
    (std::vector<std::string>{"rto4_msgs/srv/SetEncoderPosition"}));
}

TEST_F(AdapterTest, GyroscopeCallbackPublishesYawRateAndCovariances)
{
  GyroscopeROS adapter(node.get(), "robot/");
  adapter.setTimeStamp(stamp);
  auto capture = subscribe<sensor_msgs::msg::Imu>("imu");
  static_cast<api::GyroscopeExt &>(adapter).gyroscopeExtEvent(1.2f, -0.3f);
  ASSERT_TRUE(received(capture));
  const auto & message = *capture->message;
  EXPECT_EQ(message.header.stamp, stamp_message);
  EXPECT_EQ(message.header.frame_id, "robot/base_link");
  EXPECT_DOUBLE_EQ(message.orientation.x, 0.0);
  EXPECT_DOUBLE_EQ(message.orientation.y, 0.0);
  EXPECT_NEAR(message.orientation.z, std::sin(static_cast<double>(1.2f) / 2), 1e-12);
  EXPECT_NEAR(message.orientation.w, std::cos(static_cast<double>(1.2f) / 2), 1e-12);
  EXPECT_DOUBLE_EQ(message.angular_velocity.x, 0.0);
  EXPECT_DOUBLE_EQ(message.angular_velocity.y, 0.0);
  EXPECT_DOUBLE_EQ(message.angular_velocity.z, static_cast<double>(-0.3f));
  EXPECT_DOUBLE_EQ(message.linear_acceleration_covariance[0], -1.0);
  EXPECT_DOUBLE_EQ(message.orientation_covariance[8], 0.01);
  EXPECT_DOUBLE_EQ(message.angular_velocity_covariance[8], 0.01);
}

TEST_F(AdapterTest, PowerCallbackPreservesVoltageCurrentAndUsesCurrentClock)
{
  PowerManagementROS adapter(node.get());
  adapter.setTimeStamp(stamp);
  auto capture = subscribe<rto4_msgs::msg::PowerReadings>("power_readings");
  const auto before = rclcpp::Clock().now();
  static_cast<api::PowerManagement &>(adapter).readingsEvent(
    24.5f, -1.25f, true, 2, "test", true, 7);
  ASSERT_TRUE(received(capture));
  EXPECT_FLOAT_EQ(capture->message->voltage, 24.5f);
  EXPECT_FLOAT_EQ(capture->message->current, -1.25f);
  // Historical defect: PowerManagementROS ignores its assigned timestamp.
  const rclcpp::Time actual(capture->message->stamp, RCL_SYSTEM_TIME);
  EXPECT_GE(actual.nanoseconds(), before.nanoseconds());
  EXPECT_LE(actual.nanoseconds(), rclcpp::Clock().now().nanoseconds());
  EXPECT_NE(capture->message->stamp, stamp_message);
}

TEST_F(AdapterTest, BumperCallbackPublishesBothContactStates)
{
  BumperROS adapter(node.get());
  auto capture = subscribe<std_msgs::msg::Bool>("bumper");
  auto & callback = static_cast<api::Bumper &>(adapter);
  callback.bumperEvent(true);
  ASSERT_TRUE(received(capture));
  EXPECT_TRUE(capture->message->data);
  callback.bumperEvent(false);
  ASSERT_TRUE(received(capture, 1));
  EXPECT_FALSE(capture->message->data);
}

TEST_F(AdapterTest, OdometryCallbackPublishesConsistentPoseTwistFramesAndTF)
{
  OdometryROS adapter(node.get(), "robot/");
  adapter.setTimeStamp(stamp);
  auto odom = subscribe<nav_msgs::msg::Odometry>("odom");
  auto transforms = subscribe<tf2_msgs::msg::TFMessage>("/tf");
  static_cast<api::Odometry &>(adapter).readingsEvent(1.25, -2.5, 0.8, 0.3f, -0.2f, 0.1f, 7);
  ASSERT_TRUE(received(odom));
  ASSERT_TRUE(received(transforms));
  const auto & message = *odom->message;
  EXPECT_EQ(message.header.stamp, stamp_message);
  EXPECT_EQ(message.header.frame_id, "robot/odom");
  EXPECT_EQ(message.child_frame_id, "robot/base_footprint");
  EXPECT_DOUBLE_EQ(message.pose.pose.position.x, 1.25);
  EXPECT_DOUBLE_EQ(message.pose.pose.position.y, -2.5);
  EXPECT_DOUBLE_EQ(message.pose.pose.position.z, 0.0);
  EXPECT_DOUBLE_EQ(message.pose.pose.orientation.x, 0.0);
  EXPECT_DOUBLE_EQ(message.pose.pose.orientation.y, 0.0);
  EXPECT_NEAR(message.pose.pose.orientation.z, std::sin(0.4), 1e-12);
  EXPECT_NEAR(message.pose.pose.orientation.w, std::cos(0.4), 1e-12);
  EXPECT_DOUBLE_EQ(message.twist.twist.linear.x, static_cast<double>(0.3f));
  EXPECT_DOUBLE_EQ(message.twist.twist.linear.y, static_cast<double>(-0.2f));
  EXPECT_DOUBLE_EQ(message.twist.twist.linear.z, 0.0);
  EXPECT_DOUBLE_EQ(message.twist.twist.angular.x, 0.0);
  EXPECT_DOUBLE_EQ(message.twist.twist.angular.y, 0.0);
  EXPECT_DOUBLE_EQ(message.twist.twist.angular.z, static_cast<double>(0.1f));
  ASSERT_EQ(transforms->message->transforms.size(), 1u);
  const auto & transform = transforms->message->transforms[0];
  EXPECT_EQ(transform.header, message.header);
  EXPECT_EQ(transform.child_frame_id, message.child_frame_id);
  EXPECT_DOUBLE_EQ(transform.transform.translation.x, message.pose.pose.position.x);
  EXPECT_DOUBLE_EQ(transform.transform.translation.y, message.pose.pose.position.y);
  EXPECT_DOUBLE_EQ(transform.transform.translation.z, 0.0);
  EXPECT_EQ(transform.transform.rotation, message.pose.pose.orientation);
  const auto services = node->get_service_names_and_types();
  ASSERT_NE(services.find("/reset_odometry"), services.end());
  EXPECT_EQ(services.at("/reset_odometry"),
    (std::vector<std::string>{"rto4_msgs/srv/ResetOdometry"}));
  adapter.setFramePrefix("");
  static_cast<api::Odometry &>(adapter).readingsEvent(0, 0, 0, 0, 0, 0, 8);
  ASSERT_TRUE(received(odom, 1));
  EXPECT_EQ(odom->message->header.frame_id, "odom");
  EXPECT_EQ(odom->message->child_frame_id, "base_footprint");
  EXPECT_DOUBLE_EQ(odom->message->pose.pose.orientation.w, 1.0);
}

TEST_F(AdapterTest, CameraCallbackPreservesPaddedBgrRowsAndNumberedTopics)
{
  api::Com com;
  CameraROS primary;
  CameraROS secondary;
  primary.setComId(com.id());
  secondary.setComId(com.id());
  primary.setNumber(0, node.get());
  secondary.setNumber(2, node.get());
  primary.setTimeStamp(stamp);
  secondary.setTimeStamp(stamp);
  auto first = subscribe<sensor_msgs::msg::Image>("image_raw");
  auto second = subscribe<sensor_msgs::msg::Image>("image_raw2");
  const unsigned char bytes[] = {1, 2, 3, 4, 5, 6, 99, 7, 8, 9, 10, 11, 12, 98};
  static_cast<api::Camera &>(primary).imageReceivedEvent(bytes, sizeof(bytes), 2, 2, 7);
  ASSERT_TRUE(received(first));
  EXPECT_EQ(second->count, 0u);
  const auto & message = *first->message;
  EXPECT_EQ(message.header.stamp, stamp_message);
  EXPECT_EQ(message.encoding, "bgr8");
  EXPECT_EQ(message.width, 2u);
  EXPECT_EQ(message.height, 2u);
  EXPECT_EQ(message.step, 7u);
  EXPECT_EQ(message.is_bigendian, 0u);
  EXPECT_EQ(message.data, (std::vector<unsigned char>(bytes, bytes + sizeof(bytes))));
  static_cast<api::Camera &>(secondary).imageReceivedEvent(bytes, 3, 1, 1, 3);
  ASSERT_TRUE(received(second));
  EXPECT_EQ(second->message->data, (std::vector<unsigned char>{1, 2, 3}));
}

TEST_F(AdapterTest, CameraRejectsInvalidBuffersWithoutPublishing)
{
  CameraROS adapter;
  auto & callback = static_cast<api::Camera &>(adapter);
  const unsigned char bytes[] = {1, 2, 3, 4, 5, 6};
  callback.imageReceivedEvent(bytes, 6, 2, 1, 6);
  adapter.setNumber(0, node.get());
  auto capture = subscribe<sensor_msgs::msg::Image>("image_raw");
  callback.imageReceivedEvent(nullptr, 6, 2, 1, 6);
  callback.imageReceivedEvent(bytes, 5, 2, 1, 6);
  callback.imageReceivedEvent(bytes, 6, 2, 2, 6);
  callback.imageReceivedEvent(bytes, 6, 2, 1, 5);
  callback.imageReceivedEvent(bytes, 6, 0, 1, 6);
  callback.imageReceivedEvent(bytes, 6, 2, 0, 6);
  callback.imageReceivedEvent(bytes, 6, 2, 1, 0);
  const auto maximum = std::numeric_limits<unsigned int>::max();
  callback.imageReceivedEvent(bytes, maximum, maximum, 1, maximum);
  callback.imageReceivedEvent(bytes, maximum, 1, maximum, maximum);
  drain();
  EXPECT_EQ(capture->count, 0u);
  callback.imageReceivedEvent(bytes, 6, 2, 1, 6);
  ASSERT_TRUE(received(capture));
  EXPECT_EQ(capture->message->data, (std::vector<unsigned char>{1, 2, 3, 4, 5, 6}));
}
}  // namespace
