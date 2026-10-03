/*
 * OdometryROS.h
 *
 *  Created on: 07.12.2011
 *      Author: indorewala@servicerobotics.eu
 */

#ifndef ODOMETRYROS_H_
#define ODOMETRYROS_H_

#include "rec/robotino/api2/Odometry.h"
#include "rto4_msgs/srv/reset_odometry.hpp"

#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/transform_broadcaster.h"

#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

#include <string>

class OdometryROS : public rec::robotino::api2::Odometry
{
public:
	OdometryROS(rclcpp::Node *parent_node, const std::string &frame_prefix = "");
	~OdometryROS();

	void setTimeStamp(rclcpp::Time stamp);
	void setFramePrefix(const std::string &fp) { frame_prefix_ = fp; }

	// Builds the odom message + TF (no publish, no live Com needed) so perf
	// tests can time the exact production hot path off-hardware.
	void benchmarkBuildOdometry(double x, double y, double phi,
								float vx, float vy, float omega)
	{
		buildOdometryMessages(x, y, phi, vx, vy, omega);
	}

private:
	rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odometry_pub_;

	rclcpp::Service<rto4_msgs::srv::ResetOdometry>::SharedPtr reset_odometry_server_;

	nav_msgs::msg::Odometry odometry_msg_;
	geometry_msgs::msg::TransformStamped odometry_transform_;

	tf2_ros::TransformBroadcaster odometry_transform_broadcaster_;

	std::string frame_prefix_;
	rclcpp::Time stamp_;

	void buildOdometryMessages(double x, double y, double phi,
							   float vx, float vy, float omega);
	void readingsEvent(double x, double y, double phi,
					   float vx, float vy, float omega, unsigned int sequence);
	bool resetOdometryCallback(
		rto4_msgs::srv::ResetOdometry::Request::SharedPtr req,
		rto4_msgs::srv::ResetOdometry::Response::SharedPtr res);
};

#endif /* ODOMETRYROS_H_ */
