/*
 * RTOEmbStreamlineNode.cpp
 */

#include "RTOEmbStreamlineNode.h"

#include <cmath>
#include <limits>

using namespace std::chrono_literals;

OdometryStreamline::OdometryStreamline(rclcpp::Node *parent_node, const std::string &frame_prefix) : odometry_transform_broadcaster_(parent_node),
                                                                                                     frame_prefix_(frame_prefix),
                                                                                                     last_phi_(std::numeric_limits<double>::quiet_NaN()),
                                                                                                     quat_z_(0.0),
                                                                                                     quat_w_(1.0)
{
    // Shallow QoS (keep last 1): odom is a latest-value stream; a deep queue
    // just adds serialization/book-keeping cost under emulation.
    odometry_pub_ = parent_node->create_publisher<nav_msgs::msg::Odometry>(
        "odom", rclcpp::QoS(rclcpp::KeepLast(1)));

    setFramePrefix(frame_prefix_);
}

OdometryStreamline::~OdometryStreamline()
{
}

void OdometryStreamline::setFramePrefix(const std::string &fp)
{
    // Frame ids are constant for the node's lifetime — set once, never per msg.
    frame_prefix_ = fp;
    odometry_msg_.header.frame_id = frame_prefix_ + "odom";
    odometry_msg_.child_frame_id = frame_prefix_ + "base_footprint";
    odometry_transform_.header.frame_id = frame_prefix_ + "odom";
    odometry_transform_.child_frame_id = frame_prefix_ + "base_footprint";
}

void OdometryStreamline::buildOdometryMessages(double x, double y, double phi,
                                               float vx, float vy, float omega)
{
    // Yaw-only quaternion computed directly (z=sin(phi/2), w=cos(phi/2)); skips
    // the tf2 Quaternion setRPY + toMsg object churn. Cached so a (near-)constant
    // heading avoids repeated sin/cos.
    if (!(std::fabs(phi - last_phi_) < 1e-9))
    {
        quat_z_ = std::sin(phi * 0.5);
        quat_w_ = std::cos(phi * 0.5);
        last_phi_ = phi;
    }

    odometry_msg_.header.stamp = stamp_;
    odometry_msg_.pose.pose.position.x = x;
    odometry_msg_.pose.pose.position.y = y;
    odometry_msg_.pose.pose.position.z = 0.0;
    odometry_msg_.pose.pose.orientation.x = 0.0;
    odometry_msg_.pose.pose.orientation.y = 0.0;
    odometry_msg_.pose.pose.orientation.z = quat_z_;
    odometry_msg_.pose.pose.orientation.w = quat_w_;
    odometry_msg_.twist.twist.linear.x = vx;
    odometry_msg_.twist.twist.linear.y = vy;
    odometry_msg_.twist.twist.linear.z = 0.0;
    odometry_msg_.twist.twist.angular.x = 0.0;
    odometry_msg_.twist.twist.angular.y = 0.0;
    odometry_msg_.twist.twist.angular.z = omega;

    odometry_transform_.header.stamp = stamp_;
    odometry_transform_.transform.translation.x = x;
    odometry_transform_.transform.translation.y = y;
    odometry_transform_.transform.translation.z = 0.0;
    odometry_transform_.transform.rotation.x = 0.0;
    odometry_transform_.transform.rotation.y = 0.0;
    odometry_transform_.transform.rotation.z = quat_z_;
    odometry_transform_.transform.rotation.w = quat_w_;
}

void OdometryStreamline::readingsEvent(double x, double y, double phi,
                                       float vx, float vy, float omega, unsigned int sequence)
{
    (void)sequence;
    buildOdometryMessages(x, y, phi, vx, vy, omega);
    odometry_transform_broadcaster_.sendTransform(odometry_transform_);
    odometry_pub_->publish(odometry_msg_);
}

RTOEmbStreamlineNode::RTOEmbStreamlineNode() : Node("rto4_node_emb_streamline"),
                                               com_(this),
                                               omni_drive_(this),
                                               odometry_(this)
{
    this->declare_parameter("hostname", "172.26.1.1");
    this->declare_parameter("frame_prefix", "");
    this->declare_parameter("max_linear_vel", 2.3);
    this->declare_parameter("min_linear_vel", 0.02);
    this->declare_parameter("max_angular_vel", 1.0);
    this->declare_parameter("min_angular_vel", 0.1);

    hostname_ = this->get_parameter("hostname").as_string();
    max_linear_vel_ = this->get_parameter("max_linear_vel").as_double();
    min_linear_vel_ = this->get_parameter("min_linear_vel").as_double();
    max_angular_vel_ = this->get_parameter("max_angular_vel").as_double();
    min_angular_vel_ = this->get_parameter("min_angular_vel").as_double();

    std::string fp = this->get_parameter("frame_prefix").as_string();
    frame_prefix_ = fp.empty() ? "" : fp + "/";
    odometry_.setFramePrefix(frame_prefix_);

    RCLCPP_INFO(this->get_logger(),
                "Streamlined odom+drive node connecting to Robotino at %s", hostname_.c_str());

    com_.setName("EmbStreamline");

    initModules();
    timer_ = this->create_wall_timer(200ms, std::bind(&RTOEmbStreamlineNode::spin, this));
}

RTOEmbStreamlineNode::~RTOEmbStreamlineNode()
{
}

void RTOEmbStreamlineNode::initModules()
{
    com_.setAddress(hostname_.c_str());

    // Both modules share the one Com, so a single processEvents() loop services
    // odometry callbacks and drive commands together.
    omni_drive_.setComId(com_.id());
    odometry_.setComId(com_.id());
    omni_drive_.setMaxMin(max_linear_vel_, min_linear_vel_, max_angular_vel_, min_angular_vel_);

    com_.connectToServer(false);
}

void RTOEmbStreamlineNode::spin()
{
    // Reuse the node clock instead of constructing a fresh rclcpp::Clock() per
    // tick (the stock nodes do `rclcpp::Clock().now()` every spin).
    odometry_.setTimeStamp(this->now());
    com_.processEvents();
}
