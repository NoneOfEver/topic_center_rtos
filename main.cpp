/**
 * @file main.cpp
 * @brief 守护监督系统演示程序
 * 
 * 本文件演示了 DaemonSupervisor 系统的完整使用流程：
 * 1. 创建和配置 DaemonClient
 * 2. 注册到 DaemonSupervisor
 * 3. 设置故障回调和硬件看门狗
 * 4. 启动守护任务
 * 5. 模拟模块的正常运行和故障场景
 */

#ifdef __ARM_ARCH

#include "cmsis_os2.h"
#include <cstdint>
#include <cstdio>
#include "daemon_client.hpp"
#include "supervisor.hpp"
#include "SEGGER_RTT.h"
#include "Init.h"


#define LOG(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// =============================================================================
// 回调函数定义
// =============================================================================

/**
 * @brief IMU 模块故障回调
 * 
 * 当 IMU 客户端超时离线时被调用。
 * 这是模块级别的回调，用于处理特定模块的故障。
 * 
 * @param c 触发故障的客户端引用
 * 
 * 实际应用中可以：
 * - 切换到备用传感器
 * - 使用上一次的有效数据
 * - 发送告警消息
 */
void imu_fault(DaemonClient& c) {
    LOG("[Callback] IMU fault! Owner=%p\n", c.owner());
    // 实际应用：可以在这里切换到备用 IMU 或使用预测值
}

/**
 * @brief 控制模块故障回调
 * 
 * 当控制客户端超时离线时被调用。
 * 控制模块通常是 FATAL 级别，会触发系统级故障钩子。
 */
void control_fault(DaemonClient& c) {
    LOG("[Callback] CONTROL fault! Owner=%p\n", c.owner());
    // 实际应用：紧急停止电机输出
}

/**
 * @brief 系统级故障钩子
 * 
 * 当 FATAL 级别的客户端离线时被调用。
 * 这是全局的紧急处理，应该执行系统级的安全操作。
 * 
 * @param c 触发故障的客户端引用
 * 
 * 典型操作：
 * - 紧急停止所有电机
 * - 切换到安全模式
 * - 触发软件复位
 * - 保存故障日志
 */
void system_fault(DaemonClient& c) {
    LOG("[SystemHook] FATAL client offline! Owner=%p -> System reset!\n", c.owner());
    // 实际应用：
    // - 停止所有电机输出
    // - 断开功率输出
    // - 可选：触发系统复位 NVIC_SystemReset();
}

/**
 * @brief 硬件看门狗喂狗函数
 * 
 * 只有当所有 CRITICAL 优先级的客户端都在线时才会被调用。
 * 这确保了关键模块故障时，硬件看门狗会超时复位系统。
 * 
 * 实际应用中应该调用硬件看门狗外设的喂狗函数：
 * - STM32 独立看门狗: HAL_IWDG_Refresh(&hiwdg);
 * - STM32 窗口看门狗: HAL_WWDG_Refresh(&hwwdg);
 */
void hw_feed_demo() {
    LOG("[HW Feed] Hardware watchdog fed\n");
    // 实际应用：HAL_IWDG_Refresh(&hiwdg);
}

// // -----------------------------
// // CMSIS-RTOS2 daemon task (已移至 daemon_task.cpp)
// // -----------------------------
// void daemon_task(void* arg) {
//     (void)arg;
//     uint32_t now_ms = 0;
//     while(1) {
//         DaemonSupervisor::tick(now_ms);
//         now_ms += 10; // tick every 10ms -> 100Hz
//         osDelay(10);
//     }
// }

// =============================================================================
// 主函数
// =============================================================================

/**
 * @brief 应用程序主入口
 * 
 * 演示守护监督系统的完整使用流程。
 * 
 * 场景说明：
 * ──────────
 * - imu_client:  超时 50ms, DEGRADED 级别, NORMAL 优先级
 *                -> 超时只触发回调，不影响系统
 * 
 * - ctrl_client: 超时 30ms, FATAL 级别, CRITICAL 优先级
 *                -> 超时触发回调 + 系统钩子，并停止喂硬件看门狗
 * 
 * 模拟行为：
 * - IMU 每 10ms feed 一次（正常）
 * - Control 只在前 300ms 内 feed（之后超时）
 */
void app_main(void* argument){

    // ===== 第一步：初始化守护监督者 =====
    DaemonSupervisor::init();

    // 创建模拟的"拥有者"对象
    // 实际应用中这会是真正的模块对象指针
    int imu_obj = 1;
    int ctrl_obj = 2;

    // ===== 第二步：创建 DaemonClient 实例 =====
    
    /**
     * IMU 客户端配置：
     * - 超时: 50ms (需要 > 50ms 不喂狗才会离线)
     * - 回调: imu_fault
     * - 拥有者: &imu_obj (实际应用中是 IMU 模块指针)
     * - 域: SENSOR
     * - 故障等级: DEGRADED (降级运行，不触发系统钩子)
     * - 优先级: NORMAL (不影响硬件看门狗)
     */
    DaemonClient imu_client(
        50, imu_fault, &imu_obj,
        DaemonClient::Domain::SENSOR,
        DaemonClient::FaultLevel::DEGRADED,
        DaemonClient::Priority::NORMAL
    );

    /**
     * 控制客户端配置：
     * - 超时: 30ms (需要 > 30ms 不喂狗才会离线)
     * - 回调: control_fault
     * - 拥有者: &ctrl_obj
     * - 域: CONTROL
     * - 故障等级: FATAL (触发系统故障钩子)
     * - 优先级: CRITICAL (影响硬件看门狗喂狗)
     */
    DaemonClient ctrl_client(
        30, control_fault, &ctrl_obj,
        DaemonClient::Domain::CONTROL,
        DaemonClient::FaultLevel::FATAL,
        DaemonClient::Priority::CRITICAL
    );

    // ===== 第三步：注册客户端到监督者 =====
    DaemonSupervisor::register_client(&imu_client);
    DaemonSupervisor::register_client(&ctrl_client);
    
    // ===== 第四步：设置回调钩子 =====
    DaemonSupervisor::set_system_fault_hook(system_fault);
    DaemonSupervisor::set_hw_feed(hw_feed_demo);

    // ===== 第五步：启动守护任务 =====
    // daemon_task 在独立任务中周期调用 tick()
    osThreadNew(daemon_task, nullptr, nullptr);

    // ===== 第六步：模拟模块运行 =====
    // 这里模拟应用层的喂狗行为
    uint32_t now_ms = 0;
    while(1) {
        // IMU 每次都喂狗 -> 保持在线
        imu_client.feed(now_ms);
        
        // Control 只在前 300ms 内喂狗
        // 之后停止喂狗 -> 会在 30ms 后超时
        // 预期行为：
        // - 触发 control_fault 回调
        // - 触发 system_fault 系统钩子（因为是 FATAL）
        // - 停止喂硬件看门狗（因为是 CRITICAL 且离线）
        if(now_ms < 30*10) ctrl_client.feed(now_ms);

        now_ms += 10;
        LOG("Alive...\n");
        osDelay(10);
    }
}
#endif