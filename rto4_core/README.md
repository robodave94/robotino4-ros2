# rto4_core: portable source-built Robotino core

`rto4_core` now provides source-built REC RPC, Robotino RPC, fleetcom, cv_lt
and API2 libraries for this stack. ROS packages can link these targets directly
instead of relying on the legacy amd64-only vendor Debian binaries.

## Source and licensing

`vendor/rec/rpc` is imported from `rec_rpc/lib/rec/rpc` in
[BusHero/robotino_api2](https://github.com/BusHero/robotino_api2), pinned at
`9757814871aa90977c2548a8a558f4b2cb015e5d`. This is an unofficial mirror.
The original [REC license](vendor/rec/rpc/LICENSE.txt) and per-file notices
are retained. Upstream build scripts are retained for provenance, but are not
executed. The package uses target-scoped CMake and system Qt 5, not vendor
shared objects or architecture-specific flags.
Imported implementation defects are fixed: equal `ClientInfo` endpoints now
return false from their ordering comparison rather than falling off a non-void
function. Names remain excluded from endpoint ordering, as in the vendor API.
The server's worker stop flag is atomic, and worker-owned resources are also
released when shutdown occurs between event-loop iterations. The latter was
exposed by LeakSanitizer when running the lifecycle test in its own process.

The supplied REC RPC 1.6.1 public headers match the source API after comments
and whitespace are excluded (apart from the generated version header).
The candidate implementation is 1.6.0; header equivalence is not evidence
of full behavioral parity. Its version is deliberately reported as
`1.6.0-rto4`, not falsely identified as the supplied binary.

The inspected API2 source is older than the supplied API2 1.1.14. Contract
differences include controller information, omni-drive, charger, digital-output
and relay arrays. Robotino RPC also differs in output status, relay status,
charger/omni-drive schemas and JSON messages. Auxiliary Grappler SDK interfaces
require a separate audit. The candidate API2 implementation depends on CV and fleetcom code. Those
sources are imported and built in this package so the ROS stack can compile
without the vendor Debian artifacts.

## Building and testing

In a Jazzy workspace with Qt 5 development packages:

```sh
colcon build --packages-select rto4_core --cmake-args -DBUILD_TESTING=ON
colcon test --packages-select rto4_core
colcon test-result --verbose
```

The RPC foundation also builds independently of ROS:

```sh
cmake -S rto4_core -B build/core \
  -DRTO4_CORE_STANDALONE=ON -DBUILD_TESTING=ON
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

Consumers can link `rto4_core::api2` (which transitively links the core
libraries), or individual targets (`rto4_core::rpc`, `rto4_core::robotino_rpc`,
`rto4_core::fleetcom`, `rto4_core::cv_lt`). Qt5 Core/Network/Xml/Gui and JPEG
are required. No Dockerfiles are changed.

An installation/consumer smoke check is provided:

```sh
cmake --install build/core --prefix /tmp/rto4-core-install
cmake -S rto4_core/test/consumer -B build/consumer \
  -DCMAKE_PREFIX_PATH=/tmp/rto4-core-install
cmake --build build/consumer
python3 rto4_core/test/test_peers.py build/consumer/installed_peer
```

## Validation boundary

The current tests assert signed positions/floating-point vectors, rejected type
versions and truncated serializations, endpoint ordering, server lifecycle,
initial permanent-topic snapshots, topic publication, synchronous RPC results,
unknown-method errors, and peer shutdown. The separate-process peer harness can
be compiled against the supplied vendor REC RPC library and passed with
`--vendor /path/to/vendor-rpc-peer`. This compares exact fixture bytes and runs
vendor/new client/server pairs in both directions over TCP and Unix sockets.
Vendor packages are not required for ordinary package builds/tests.

On the inspected amd64 Jazzy container these tests also pass under ASan/UBSan.
That result is not coverage of all REC RPC operations, ARM execution, API2,
Robotino ROS transport, or hardware. Shared-memory topics are not covered by the
passing peer suite: a separate 4096-byte shared-memory snapshot experiment failed
with both vendor/vendor and source/source peers and requires characterization.
Do not treat local Unix-socket success as shared-memory conformance.

## Release gates

- Reconcile every API2 C++/C and Robotino RPC interface with the supplied SDK.
- Establish licenses for every required imported implementation.
- Validate vendor/new client/server interoperability in both directions.
- Test camera/laser/peripheral transport and error/lifetime behavior.
- Migrate all five ROS executables without changing their interfaces.
- Add the agreed 0.5-second monotonic command watchdog to both drive nodes,
  test deadline boundaries and prevent motion replay after reconnect.
- Build every ROS package and link actual amd64 and arm64 executables;
  distinguish cross compilation from native ARM test execution.
- Keep hardware commissioning safeguards in place while validating behavior on
  real robots.

Hardware-free checks cannot confirm firmware-specific behavior, calibration,
emergency stop, physical braking, or a stop after the network has failed.
Commission with wheels raised, an accessible emergency stop, verified firmware,
sensor/odometry comparisons, and explicit watchdog/network-loss trials before
floor operation. SDK availability must not automatically enable unused
actuators in ROS bringup.
