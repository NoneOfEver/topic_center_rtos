#pragma once

#include "message_center.hpp"
#include <vector>
#include <string>
#include <memory>
#include "cmsis_os2.h" // 引入 CMSIS-OS2 API

/**
 * @class Topic
 * @brief 模板化的具体话题实现。
 * @tparam T 消息的数据类型。
 */
template <typename T>
class Topic : public ITopic {
public:
    Topic(const std::string& name) : name_(name) {
        topic_mutex_ = osMutexNew(NULL);
    }

    ~Topic() {
        osMutexDelete(topic_mutex_);
    }

    const std::string& getName() const override {
        return name_;
    }

    void addSubscriber(std::shared_ptr<ISubscriber> sub) override {
        osMutexAcquire(topic_mutex_, osWaitForever);
        subscribers_.push_back(sub);
        osMutexRelease(topic_mutex_);
    }

    void publish(const void* data) override {
        osMutexAcquire(topic_mutex_, osWaitForever);
        std::vector<std::shared_ptr<ISubscriber>> subs_copy = subscribers_;
        osMutexRelease(topic_mutex_);

        for (auto& sub : subs_copy) {
            sub->push(data);
        }
    }

private:
    std::string name_;
    std::vector<std::shared_ptr<ISubscriber>> subscribers_;
    osMutexId_t topic_mutex_; // 使用 osMutexId_t
};

/**
 * @class Subscriber
 * @brief 模板化的具体订阅者实现，使用消息队列。
 * @tparam T 消息的数据类型。
 */
template <typename T>
class Subscriber : public ISubscriber {
public:
    Subscriber(size_t queue_size = 1) {
        queue_ = osMessageQueueNew(queue_size, sizeof(T), NULL);
    }

    ~Subscriber() {
        osMessageQueueDelete(queue_);
    }

    void push(const void* data) override {
        // 阻塞直到消息放入队列（确保生产者在消费者取走消息前不会丢失）
        osMessageQueuePut(queue_, data, 0, osWaitForever);
    }

    bool getMessage(T& data, uint32_t timeout = osWaitForever) {
        // 从队列中获取消息，可以设置超时
        return osMessageQueueGet(queue_, &data, NULL, timeout) == osOK;
    }

private:
    osMessageQueueId_t queue_; // 使用 CMSIS-OS2 消息队列
};

/**
 * @class Publisher
 * @brief 模板化的发布者，用于发布特定类型的话题。
 * @tparam T 消息的数据类型。
 */
template <typename T>
class Publisher {
public:
    Publisher(const std::string& topic_name) {
        MessageCenter& center = MessageCenter::getInstance();
        std::shared_ptr<ITopic> base_topic = center.getTopic(topic_name);
        if (base_topic) {
            // 话题已存在，直接使用 static_cast（假设类型正确）
            topic_ = std::static_pointer_cast<Topic<T>>(base_topic);
        } else {
            // 话题不存在，创建新话题
            topic_ = std::make_shared<Topic<T>>(topic_name);
            center.addTopic(topic_);
        }
    }

    void publish(const T& data) {
        topic_->publish(&data);
    }

private:
    std::shared_ptr<Topic<T>> topic_;
};

// Helper function to create a subscriber and register it to a topic
template <typename T>
std::shared_ptr<Subscriber<T>> subscribe(const std::string& topic_name, size_t queue_size = 1) {
    MessageCenter& center = MessageCenter::getInstance();
    auto sub = std::make_shared<Subscriber<T>>(queue_size);
    std::shared_ptr<ITopic> topic = center.getTopic(topic_name);
    if (!topic) {
        // 如果主题不存在，创建一个新的
        topic = std::make_shared<Topic<T>>(topic_name);
        center.addTopic(topic);
    }
    topic->addSubscriber(sub);
    return sub;
}
