# Hardware-free adapter baseline

`test_rto4_adapters` compiles the production ROS adapters and invokes callbacks
through their public API2 base classes. This exercises virtual dispatch without
opening a Robotino connection. ROS subscriptions check the published messages;
the odometry test also receives `/tf`. The camera-number test owns an
unconnected API2 `Com`, because changing camera numbers requires a valid Com ID.

Coverage includes valid velocity bands (including signed zero), sensor values,
array ordering and resizing, timestamps, frame prefixes, yaw quaternions,
covariances, odometry/TF consistency, service names/types, padded BGR images, and
invalid camera buffers. Joint-state regressions exercise the same conversion
helper used by `RTONode`, including empty and independently arriving motor
arrays. Joint states are withheld until both arrays contain all three wheels.
The motor-position regression calls the vendor's `const int*` virtual method.

Run in a ROS environment with the vendor SDK available:

```bash
colcon build --packages-up-to rto4_node --cmake-args -DBUILD_TESTING=ON
colcon test --packages-select rto4_node \
  --ctest-args -R 'test_rto4_(adapters|perf)$' --output-on-failure
colcon test-result --verbose
```

For extracted SDKs, provide `-DCMAKE_MODULE_PATH=/path/to/api2/cmake` and
`-DROBOTINOAPI2_DIR=/path/to/api2`; expose vendor shared-library directories in
`LD_LIBRARY_PATH`. Use separate colcon build/install/log directories when
comparing vendors or implementations. No host package installation is required
when these tests run in an existing ROS development container.

## Historical defects characterized, not endorsed

* `EncoderReadings` uses unsigned integers for velocity, position, and current:
  negative ticks wrap and fractional current truncates. The baseline explicitly
  records these existing schema conversions; it does not change the interface.
* Power readings ignore `setTimeStamp` and use the current system clock.
* Distance geometry retains the original approximate `0.698` radian spacing
  and `0.2` metre radius, and joint conversions retain the original `3.142` pi
  approximation.

These tests do **not** establish transport compatibility, firmware support,
physical sensor calibration, actuator effects of services/commands, watchdog
behavior, or safety. All ROS message exchanges here are within one process.
The existing performance test remains a benchmark, not functional evidence.
