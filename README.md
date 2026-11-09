# RTO-ROS 2


This repository contains code for packages that enable ROS 2 support for the **Festo Robotino 3**.

This repo is verified to work on Ubuntu 20.04 LTS with ROS2 Foxy and Ubuntu 24.04 LTS with ROS2 Jazzy.

The original code was taken from [dietriro's robotino repository](https://github.com/dietriro/rto4_core) and adapted for ROS2
This fork was originally from [Rahul-a-k's repo](https://github.com/rahul-k-a/robotino-ros2)
In addition, this stack does not use any python, only cpp and xml. The foundation for all development here was possible here is thanks to [Rahul-a-k (forked repo)](https://github.com/rahul-k-a/robotino-ros2) and also [Elena Villalba's repository](https://github.com/elena-villalba/robotino4-ros2), who developed a stack for the robotino4 and nav w/ Python. 

**Note:** Not all functionality has been enabled. North Star support, BHA support, and gripper support have been disabled.

## Portable core migration

The stack now builds against the in-tree [rto4_core package](rto4_core/README.md),
which provides source-built REC RPC, Robotino RPC, fleetcom, cv_lt and API2
libraries. This removes the previous amd64-only `.deb` runtime dependency from
the ROS build path and allows compiling the stack for alternate architectures.
Hardware validation is still required before field operation.

## Installation instructions for Host PC
- Clone this repo into a ros2 workspace
- Install build prerequisites (Qt 5 and JPEG development packages): `sudo apt-get install qtbase5-dev qtdeclarative5-dev libqt5xmlpatterns5-dev libjpeg-dev`
- Install `teleop_twist_keyboard`: `sudo apt-get install ros-${ROS_DISTRO}-teleop-twist-keyboard`
- Do `colcon build`
- Source the workspace

## Running the robot

### 1. Bring up the robot

```bash
ros2 launch rto4_bringup rto4_bringup.launch hostname:=<robotino_ip> [ns:=rto3]
```

| Argument | Default | Description |
|---|---|---|
| `hostname` | `172.26.1.1` | Robotino IP address |
| `ns` | `rto3` | ROS namespace for the robot nodes |

### 2. Launch keyboard teleop

In a separate terminal:

```bash
ros2 launch rto4_bringup rto4_teleop.launch [ns:=rto3]
```

The `ns` argument must match the namespace used in the bringup launch. `teleop_twist_keyboard` will open in the same terminal — use the standard keys to drive:

| Key | Action |
|---|---|
| `i` / `,` | Forward / Backward |
| `j` / `l` | Rotate left / right |
| `u` / `o` | Strafe diagonal |
| `k` | Stop |
| `q` / `z` | Increase / decrease max speed |
| `w` / `x` | Increase / decrease linear speed only |
| `e` / `c` | Increase / decrease angular speed only |

### 3. Launch RViz visualisation

```bash
ros2 launch rto4_bringup rto4_rviz.launch
```

## Description package layout

`rto4_description` exposes the model in two files so it can be reused by
composition xacros (e.g. mounting external modules like the SwitchLinq RVR):

| File | Role |
|---|---|
| `urdf/rover/robotino4.macro.xacro` | Defines the `robotino4` xacro:macro. Include this from composition xacros. Does NOT instantiate the macro. |
| `urdf/rover/base.urdf.xacro`       | Thin standalone wrapper. Declares a single `prefix` arg and invokes the macro. Use this from launch files, RViz, and simulation. |

### Mounting frames

The macro publishes an extra TF frame for external hardware to bolt to:

| Frame | Parent | Offset (xyz) | Purpose |
|---|---|---|---|
| `lower_right_mounting_bolt` | `base_footprint` | `0.0622  -0.0215  0.056` | Reference for modules mounted on the lower-right of the chassis (e.g. the SwitchLinq RVR racking module) |

This replaces the historical
`ros2 run tf2_ros static_transform_publisher --x 0.0622 --y -0.0215 --z 0.056 --frame-id base_footprint --child-frame-id lower_right_mounting_bolt`
side-car process — the frame is now part of the published `robot_description`
so any downstream consumer (RViz, tf2_ros, MoveIt, …) sees it automatically.

## Velocity limits

Default velocity limits configured in `rto4_node`:

| Parameter | Default | Description |
|---|---|---|
| `max_linear_vel` | `2.3` m/s | Maximum linear speed (x and y axes) |
| `min_linear_vel` | `0.02` m/s | Minimum non-zero linear speed (deadband) |
| `max_angular_vel` | `1.0` rad/s | Maximum angular (rotation) speed |
| `min_angular_vel` | `0.1` rad/s | Minimum non-zero angular speed (deadband) |

These can be overridden at launch:

```bash
ros2 launch rto4_bringup rto4_bringup.launch max_linear_vel:=1.0 max_angular_vel:=0.5
```

## TODO::
<!-- (ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args --remap cmd_vel:=/rto3/cmd_vel) -->

## Todo:
 - Odom and sensors implemented, but need to test/evaluate that namespace configs work ok, and using teleop is also ok from the odom stack side of things
 - build launch file to evaluate namespace config differently
 - test that bumper sensor runs on bringup through the topic
 - Test various topics running ok
 - test power supply readings are running on stack through topic publishing
 - Test/upgrade the rviz file to additionally read the namespaces properly
