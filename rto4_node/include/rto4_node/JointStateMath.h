/*
 * JointStateMath.h
 *
 * Robotino motor-tick -> wheel-joint conversions, shared by RTONode and the
 * performance tests so the benchmarked math cannot drift from production.
 */

#ifndef RTO4_JOINT_STATE_MATH_H_
#define RTO4_JOINT_STATE_MATH_H_

#include <vector>

namespace rto4
{
    // Gear ratio 16; constant kept at the historical 3.142 pi approximation so the
    // published joint states are byte-for-byte unchanged by this refactor.
    inline double motorVelToWheelRad(double motor_velocity)
    {
        return (motor_velocity / 16) * (2 * 3.142) / 60;
    }

    inline double motorPosToWheelRad(double motor_position)
    {
        return (motor_position / 16) * (2 * 3.142);
    }

    inline bool populateWheelJointState(
        const std::vector<float>& motor_velocities,
        const std::vector<int>& motor_positions,
        std::vector<double>& wheel_velocities,
        std::vector<double>& wheel_positions)
    {
        // Velocity and position callbacks can arrive independently at startup.
        if (motor_velocities.size() < 3 || motor_positions.size() < 3)
            return false;

        wheel_velocities.resize(3);
        wheel_positions.resize(3);
        const unsigned int motor_indices[] = {2, 0, 1};
        for (unsigned int i = 0; i < 3; ++i)
        {
            wheel_velocities[i] = motorVelToWheelRad(motor_velocities[motor_indices[i]]);
            wheel_positions[i] = motorPosToWheelRad(motor_positions[motor_indices[i]]);
        }
        return true;
    }
} // namespace rto4

#endif /* RTO4_JOINT_STATE_MATH_H_ */
