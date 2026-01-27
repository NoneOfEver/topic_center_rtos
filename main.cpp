#include "cmsis_os2.h"
#include "uorb_lite/uorb_topics.hpp"
#include "uorb_lite/uorb_lite.hpp"
#include <cstdio>
#include <cstdint>
#include <atomic>
#include <limits>
#include <SEGGER_RTT.h>
#include "Init.h"
#include "main.h"

#define LOG(...) SEGGER_RTT_printf(0, __VA_ARGS__)

/*========================
  配置
========================*/
constexpr int NUM_PUBLISHERS = 2;
constexpr int NUM_SUBSCRIBERS = 3;
constexpr int PUBLISH_RATE_HZ = 1000;  // 每秒 1000 次
constexpr int RING_DEPTH = 32;

/*========================
  Topic 定义
========================*/
struct TestData {
    uint32_t counter;
    uint32_t timestamp_us;
};

inline UORBRingTopic<TestData, RING_DEPTH> test_topic;

/*========================
  Subscriber 状态结构
========================*/
struct SubscriberStat {
    std::atomic<uint32_t> received{0};
    std::atomic<uint32_t> lost{0};
    std::atomic<uint32_t> latency_sum{0};
    std::atomic<uint32_t> latency_min{std::numeric_limits<uint32_t>::max()};
    std::atomic<uint32_t> latency_max{0};
};

SubscriberStat sub_stats[NUM_SUBSCRIBERS];

/*========================
  高精度计数函数
========================*/
inline uint32_t get_microseconds() {
#ifdef __ARM_ARCH
    // STM32 用 DWT->CYCCNT
    return DWT->CYCCNT / (SystemCoreClock / 1000000);
#else
    // SITL / PC 模拟
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count()
    );
#endif
}

/*========================
  发布者 Task
========================*/
void PublisherTask(void* arg)
{
    int id = (int)(intptr_t)arg;
    TestData data{};
    uint32_t counter = 0;
    const uint32_t period_us = 1000000 / PUBLISH_RATE_HZ;

    while (1) {
        uint32_t start_us = get_microseconds();

        data.counter = counter++;
        data.timestamp_us = start_us;

        test_topic.publish(data);

        // 控制发布频率
        while ((get_microseconds() - start_us) < period_us) {
            osThreadYield();  // 让出 CPU
        }
    }
}

/*========================
  订阅者 Task
========================*/
void SubscriberTask(void* arg)
{
    int id = (int)(intptr_t)arg;
    UORBRingSub<TestData, RING_DEPTH> sub(test_topic);

    uint32_t last_counter = 0;

    while (1) {
        TestData data;
        while (sub.copy(data)) {
            sub_stats[id].received.fetch_add(1);

            // 丢帧检测
            if (last_counter != 0 && data.counter != last_counter + 1) {
                uint32_t lost_frames = (data.counter > last_counter + 1) ? data.counter - last_counter - 1 : 0;
                sub_stats[id].lost.fetch_add(lost_frames);
            }

            // 延迟统计（注意：timestamp_us 使用微秒）
            uint32_t latency = get_microseconds() - data.timestamp_us; // microseconds
            sub_stats[id].latency_sum.fetch_add(latency);

            // 最大延迟
            uint32_t prev_max = sub_stats[id].latency_max.load();
            while (latency > prev_max &&
                   !sub_stats[id].latency_max.compare_exchange_weak(prev_max, latency)) {
                // retry
            }

            // 最小延迟
            uint32_t prev_min = sub_stats[id].latency_min.load();
            while (latency < prev_min &&
                   !sub_stats[id].latency_min.compare_exchange_weak(prev_min, latency)) {
                // retry
            }

            last_counter = data.counter;
        }
        osThreadYield(); // 避免完全 busy loop
    }
}

/*========================
  监控 Task
========================*/
void MonitorTask(void* arg)
{
    while (1) {
        osDelay(1000); // 每秒统计一次

        LOG("===== Performance Monitor =====\n");
        for (int i = 0; i < NUM_SUBSCRIBERS; ++i) {
            uint32_t received = sub_stats[i].received.exchange(0);
            uint32_t lost = sub_stats[i].lost.exchange(0);
            uint32_t sum_latency = sub_stats[i].latency_sum.exchange(0); // microseconds
            uint32_t min_latency = sub_stats[i].latency_min.exchange(std::numeric_limits<uint32_t>::max()); // microseconds
            uint32_t max_latency = sub_stats[i].latency_max.exchange(0); // microseconds

            // 避免使用浮点打印（嵌入式 printf 常常不支持 %f），改用整数和字符串缓冲格式化输出
            uint32_t total = received + lost;
            // 以万分比表示丢包率，保留两位小数（例如 12.34%% -> 1234）
            uint32_t lost_permyriad = total ? (uint32_t)((uint64_t)lost * 10000 / total) : 0;
            // 平均延迟单位为微秒（us）
            uint32_t avg_us = received ? (uint32_t)(sum_latency / received) : 0;

            char lost_rate_buf[16];
            char avg_buf[24];
            char min_buf[24];
            char max_buf[24];

            // 生成字符串，示例格式："12.34%"、"3000us"
            snprintf(lost_rate_buf, sizeof(lost_rate_buf), "%u.%02u%%", lost_permyriad / 100, lost_permyriad % 100);
            snprintf(avg_buf, sizeof(avg_buf), "%u us", avg_us);

            if (received == 0) {
                snprintf(min_buf, sizeof(min_buf), "N/A");
                snprintf(max_buf, sizeof(max_buf), "N/A");
            } else {
                // min_latency/max_latency 单位已为微秒
                snprintf(min_buf, sizeof(min_buf), "%u us", min_latency);
                snprintf(max_buf, sizeof(max_buf), "%u us", max_latency);
            }

            LOG("Subscriber %d: recv=%u, lost=%u, lost_rate=%s, latency: avg=%s, min=%s, max=%s\n",
                i, received, lost, lost_rate_buf, avg_buf, min_buf, max_buf);
        }
        LOG("\n");
    }
}

/*========================
  Main
========================*/
void app_main(void* argument)
{
    // 创建发布者
    for (int i = 0; i < NUM_PUBLISHERS; ++i) {
        osThreadNew(PublisherTask, (void*)(intptr_t)i, NULL);
    }

    // 创建订阅者
    for (int i = 0; i < NUM_SUBSCRIBERS; ++i) {
        osThreadNew(SubscriberTask, (void*)(intptr_t)i, NULL);
    }

    // 创建监控任务
    osThreadNew(MonitorTask, nullptr, NULL);

    while (1);
}
