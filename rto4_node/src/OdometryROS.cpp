/*
 * OdometryROS.cpp
 *
 *  Created on: 07.12.2011
 *      Author: indorewala@servicerobotics.eu
 */

#include "OdometryROS.h"
#include "tf2/transform_datatypes.h"
#include "tf2/buffer_core.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "geometry_msgs/msg/quaternion.hpp"

using std::placeholders::_1;
using std::placeholders::_2;

OdometryROS::OdometryROS(rclcpp::Node *parent_node, const std::string &frame_prefix) : odometry_transform_broadcaster_(parent_node),
																					   frame_prefix_(frame_prefix)
{
	odometry_pub_ = parent_node->create_publisher<nav_msgs::msg::Odometry>("odom", 10);

	reset_odometry_server_ = parent_node->create_service<rto4_msgs::srv::ResetOdometry>("reset_odometry", std::bind(&OdometryROS::resetOdometryCallback, this, _1, _2));
}

OdometryROS::~OdometryROS()
{
}

void OdometryROS::setTimeStamp(rclcpp::Time stamp)
{
	stamp_ = stamp;
}

auto createQuaternionMsgFromYaw(double yaw)
{
	tf2::Quaternion q;
	q.setRPY(0, 0, yaw);
	return tf2::toMsg(q);
}

void OdometryROS::buildOdometryMessages(double x, double y, double phi,
										float vx, float vy, float omega)
{
	geometry_msgs::msg::Quaternion phi_quat = createQuaternionMsgFromYaw(phi);

	// Construct messages
	odometry_msg_.header.frame_id = frame_prefix_ + "odom";
	odometry_msg_.header.stamp = stamp_;
	odometry_msg_.child_frame_id = frame_prefix_ + "base_footprint";
	odometry_msg_.pose.pose.position.x = x;
	odometry_msg_.pose.pose.position.y = y;
	odometry_msg_.pose.pose.position.z = 0.0;
	odometry_msg_.pose.pose.orientation = phi_quat;
	odometry_msg_.twist.twist.linear.x = vx;
	odometry_msg_.twist.twist.linear.y = vy;
	odometry_msg_.twist.twist.linear.z = 0.0;
	odometry_msg_.twist.twist.angular.x = 0.0;
	odometry_msg_.twist.twist.angular.y = 0.0;
	odometry_msg_.twist.twist.angular.z = omega;

	odometry_transform_.header.frame_id = frame_prefix_ + "odom";
	odometry_transform_.header.stamp = odometry_msg_.header.stamp;
	odometry_transform_.child_frame_id = frame_prefix_ + "base_footprint";
	odometry_transform_.transform.translation.x = x;
	odometry_transform_.transform.translation.y = y;
	odometry_transform_.transform.translation.z = 0.0;
	odometry_transform_.transform.rotation = phi_quat;
}

void OdometryROS::readingsEvent(double x, double y, double phi,
								float vx, float vy, float omega, unsigned int sequence)
{
	(void)sequence;
	buildOdometryMessages(x, y, phi, vx, vy, omega);

	odometry_transform_broadcaster_.sendTransform(odometry_transform_);

	// Publish the msg
	odometry_pub_->publish(odometry_msg_);
}

bool OdometryROS::resetOdometryCallback(
	rto4_msgs::srv::ResetOdometry::Request::SharedPtr req,
	rto4_msgs::srv::ResetOdometry::Response::SharedPtr res)
{
	(void)res;
	set(req->x, req->y, req->phi, true);

	return true;
}
