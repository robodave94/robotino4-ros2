/*
 * CameraROS.cpp
 *
 *  Created on: 08.12.2011
 *      Author: indorewala@servicerobotics.eu
 */

#include "CameraROS.h"
#include "sensor_msgs/fill_image.hpp"

namespace sensor_msgs
{
extern bool fillImage(
  msg::Image & image,
  const std::string & encoding_arg,
  uint32_t rows_arg,
  uint32_t cols_arg,
  uint32_t step_arg,
  const void * data_arg);
};


CameraROS::CameraROS()
{
	img_msg_ = sensor_msgs::msg::Image();
}

CameraROS::~CameraROS()
{
}


void CameraROS::setNumber( int number, rclcpp::Node * parent_node )
{
	std::stringstream topic;

	if( number == 0)
		topic << "image_raw";
	else
		topic << "image_raw" << number;

	streaming_pub_ = parent_node->create_publisher<sensor_msgs::msg::Image>(topic.str(),10);

	setCameraNumber( number );
}

void CameraROS::setTimeStamp(rclcpp::Time stamp)
{
	stamp_ = stamp;
}

void CameraROS::imageReceivedEvent(
		const unsigned char* data,
		unsigned int dataSize,
		unsigned int width,
		unsigned int height,
		unsigned int step )
{
	if (!streaming_pub_ || !data || width == 0 || height == 0 || step == 0)
		return;

	// Division avoids overflow while checking BGR row size and the complete buffer.
	if (width > step / 3 || height > dataSize / step)
		return;

	// Build the Image msg
	img_msg_.header.stamp = stamp_;
	if (!sensor_msgs::fillImage(img_msg_, "bgr8", height, width, step, data))
		return;

	// Publish the Image & CameraInfo msgs
	streaming_pub_->publish(img_msg_);

}
