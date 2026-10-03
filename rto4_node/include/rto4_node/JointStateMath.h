/*
 * JointStateMath.h
 *
 * Robotino motor-tick -> wheel-joint conversions, shared by RTONode and the
 * performance tests so the benchmarked math cannot drift from production.
 */

#ifndef RTO4_JOINT_STATE_MATH_H_
#define RTO4_JOINT_STATE_MATH_H_

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
} // namespace rto4

#endif /* RTO4_JOINT_STATE_MATH_H_ */
