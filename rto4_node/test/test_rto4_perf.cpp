/*
 * test_rto4_perf.cpp
 *
 * CPU performance benchmarks for the Robotino driver hot paths, run via
 * `colcon test`. They are INDEPENDENT of whether a Robotino is present: each
 * exercises only the on-CPU message-building / math that runs every control
 * tick, with no live Com connection. On the amd64/QEMU bridge image these
 * numbers tell us whether emulation can sustain the 5 Hz (200 ms) loop budget.
 *
 * By default the test only measures and prints a summary table (always passes).
 * Set RTO4_PERF_ASSERT=1 to also fail if any op blows a generous per-tick
 * budget. RTO4_PERF_ITERS overrides the iteration count (default 100000).
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"

#include "OdometryROS.h"
#include "OmniDriveROS.h"
#include "RTOEmbStreamlineNode.h"
#include "JointStateMath.h"

namespace
{
    size_t perf_iters()
    {
        const char *e = std::getenv("RTO4_PERF_ITERS");
        if (e)
        {
            long v = std::atol(e);
            if (v > 0)
                return static_cast<size_t>(v);
        }
        return 100000;
    }

    bool perf_assert()
    {
        const char *e = std::getenv("RTO4_PERF_ASSERT");
        return e && std::string(e) == "1";
    }

    struct Stats
    {
        double mean_ns;
        double median_ns;
        double p95_ns;
        double max_ns;
    };

    template <typename F>
    Stats time_loop(size_t iters, F &&fn)
    {
        std::vector<double> samples;
        samples.reserve(iters);
        for (size_t i = 0; i < iters; ++i)
        {
            auto t0 = std::chrono::steady_clock::now();
            fn(i);
            auto t1 = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
        }
        std::sort(samples.begin(), samples.end());
        double sum = 0.0;
        for (double s : samples)
            sum += s;
        Stats st;
        st.mean_ns = sum / static_cast<double>(samples.size());
        st.median_ns = samples[samples.size() / 2];
        st.p95_ns = samples[static_cast<size_t>(samples.size() * 0.95)];
        st.max_ns = samples.back();
        return st;
    }

    void print_header()
    {
        std::printf("\n=== RTO4 PERF SUMMARY ===\n");
        std::printf("%-28s %12s %12s %12s %12s %12s\n",
                    "operation", "mean_ns", "median_ns", "p95_ns", "max_ns", "est_rate_hz");
    }

    void print_row(const char *name, const Stats &s)
    {
        double hz = s.mean_ns > 0.0 ? 1e9 / s.mean_ns : 0.0;
        std::printf("%-28s %12.1f %12.1f %12.1f %12.1f %12.0f\n",
                    name, s.mean_ns, s.median_ns, s.p95_ns, s.max_ns, hz);
    }
} // namespace

TEST(Rto4Perf, HotPaths)
{
    const size_t iters = perf_iters();
    const bool do_assert = perf_assert();

    if (!rclcpp::ok())
    {
        rclcpp::init(0, nullptr);
    }
    auto node = std::make_shared<rclcpp::Node>("rto4_perf_bench");

    OdometryROS odom_stock(node.get(), "");
    odom_stock.setTimeStamp(node->now());

    OdometryStreamline odom_stream(node.get(), "");
    odom_stream.setTimeStamp(node->now());

    OmniDriveROS drive(node.get());
    drive.setMaxMin(2.3, 0.02, 1.0, 0.1);

    print_header();

    // 1) Stock odometry message + TF build (tf2 quaternion path).
    Stats s_odom = time_loop(iters, [&](size_t i)
                             {
		double phi = 0.001 * static_cast<double>(i);
		odom_stock.benchmarkBuildOdometry(
			0.01 * i, -0.02 * i, phi, 0.3f, 0.0f, 0.1f); });
    print_row("odometry_build_stock", s_odom);

    // 2) Streamlined odometry build (direct yaw quaternion, cached heading).
    Stats s_odom_s = time_loop(iters, [&](size_t i)
                               {
		double phi = 0.001 * static_cast<double>(i);
		odom_stream.benchmarkBuildOdometry(
			0.01 * i, -0.02 * i, phi, 0.3f, 0.0f, 0.1f); });
    print_row("odometry_build_streamline", s_odom_s);

    // 3) cmd_vel clamping (the move/operation hot path before setVelocity).
    Stats s_clamp = time_loop(iters, [&](size_t i)
                              {
		double lx = (i % 7) - 3.0;   // spans below-min, in-band and over-max
		double ly = 1.5 - (i % 5);
		double az = 0.5 - (i % 3);
		drive.clampVelocities(lx, ly, az); });
    print_row("cmd_vel_clamp", s_clamp);

    // 4) Joint-state tick math (per-wheel velocity + position conversion).
    Stats s_js = time_loop(iters, [&](size_t i)
                           {
		volatile double sink = 0.0;
		sink += rto4::motorVelToWheelRad(static_cast<double>(i % 1000));
		sink += rto4::motorPosToWheelRad(static_cast<double>(i % 1000));
		(void)sink; });
    print_row("joint_state_math", s_js);

    std::printf("\nLoop budget at 5 Hz = 200000000 ns/tick. Each op above runs"
                " once (odom/joint) or per cmd_vel message per tick.\n");
    std::printf("=== END RTO4 PERF SUMMARY ===\n\n");
    std::fflush(stdout);

    EXPECT_GT(iters, 0u);

    if (do_assert)
    {
        // Generous per-tick budgets; opt-in so native dev `colcon test` stays
        // green while the QEMU evaluation can enforce a go/no-go threshold.
        EXPECT_LT(s_odom.median_ns, 5000000.0) << "stock odometry build too slow for 5 Hz";
        EXPECT_LT(s_odom_s.median_ns, 5000000.0) << "streamline odometry build too slow for 5 Hz";
        EXPECT_LT(s_clamp.median_ns, 1000000.0) << "cmd_vel clamp too slow";
        EXPECT_LT(s_js.median_ns, 1000000.0) << "joint-state math too slow";
    }

    rclcpp::shutdown();
}
