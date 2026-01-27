#pragma once

#include "uorb_lite.hpp"

/*
 * 在这里集中定义所有 topic 类型
 * 等价 PX4 ORB_DEFINE
 *
 * 每个 topic = 静态全局实例
 */

namespace orb {

/* ======================
 * 示例数据结构
 * 你可以替换为自己的
 * ====================== */

struct ImuData {
    float accel[3];
    float gyro[3];
};

struct MotorCmd {
    float duty[4];
};

/* ======================
 * Topic instances
 * ====================== */

inline UORBMultiTopic<UORBRingTopic<ImuData, 4>, 2> imu;
inline UORBTopic<MotorCmd> motor_cmd;

} // namespace orb
