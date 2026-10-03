/*
 * RTOEmbStreamlineNode.h
 *
 * Combined, QEMU-optimized Robotino odometry + drive node (rto4_node_emb_
 * streamline). Merges what rto4_core (drive) and rto4_odometry publish into ONE
 * process over a SINGLE Com connection / processEvents loop, which roughly
 * halves the per-process emulation + networking overhead of running the two
 * stock nodes separately on the amd64/QEMU bridge. Odometry uses a lean path:
 * direct yaw-only quaternion (no tf2 setRPY/toMsg), cached heading, preallocated
 * reused messages, shallow QoS, and the node clock reused each tick.
 */

#ifndef RTO_EMB_STREAMLINE_NODE_H_
#define RTO_EMB_STREAMLINE_NODE_H_

#include "ComROS.h"
#include "OmniDriveROS.h"

#include "rec/robotino/api2/Odometry.h"

#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/transform_broadcaster.h"

#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"

#include <string>

class OdometryStreamline : public rec::robotino::api2::Odometry
{
public:
    OdometryStreamline(rclcpp::Node *parent_node, const std::string &frame_prefix = "");
    ~OdometryStreamline();

    void setTimeStamp(const rclcpp::Time &stamp) { stamp_ = stamp; }
    void setFramePrefix(const std::string &fp);

    // No-publish variant so perf tests can time the build hot path off-hardware.
    void benchmarkBuildOdometry(double x, double y, double phi,
                                float vx, float vy, float omega)
    {
        buildOdometryMessages(x, y, phi, vx, vy, omega);
    }

private:
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odometry_pub_;
    tf2_ros::TransformBroadcaster odometry_transform_broadcaster_;

    nav_msgs::msg::Odometry odometry_msg_;
    geometry_msgs::msg::TransformStamped odometry_transform_;

    std::string frame_prefix_;
    rclcpp::Time stamp_;

    // Quaternion cache: recompute sin/cos only when yaw actually moves.
    double last_phi_;
    double quat_z_, quat_w_;

    void buildOdometryMessages(double x, double y, double phi,
                               float vx, float vy, float omega);
    void readingsEvent(double x, double y, double phi,
                       float vx, float vy, float omega, unsigned int sequence) override;
};

class RTOEmbStreamlineNode : public rclcpp::Node
{
public:
    RTOEmbStreamlineNode();
    ~RTOEmbStreamlineNode();

private:
    rclcpp::TimerBase::SharedPtr timer_;
    std::string hostname_;
    std::string frame_prefix_;
    double max_linear_vel_, min_linear_vel_, max_angular_vel_, min_angular_vel_;

    ComROS com_;
    OmniDriveROS omni_drive_;
    OdometryStreamline odometry_;

    void initModules();
    void spin();
};

#endif /* RTO_EMB_STREAMLINE_NODE_H_ */
