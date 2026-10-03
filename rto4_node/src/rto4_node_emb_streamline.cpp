/*
 * rto4_node_emb_streamline.cpp
 *
 * Entry point for the combined, QEMU-optimized odometry + drive node.
 */

#include "RTOEmbStreamlineNode.h"

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RTOEmbStreamlineNode>());
    rclcpp::shutdown();
    return 0;
}
