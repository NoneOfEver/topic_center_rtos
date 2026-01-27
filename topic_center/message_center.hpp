#pragma once

#include <string>
#include <vector>
#include <memory>
#include "cmsis_os2.h" // 引入 CMSIS-OS2 API

// 前向声明
class ISubscriber;
class ITopic;

/**
 * @class MessageCenter
 * @brief 消息中心的单例实现，管理所有的话题。
 */
class MessageCenter {
public:
    /**
     * @brief 获取 MessageCenter 的单例实例。
     * @return MessageCenter& 返回单例实例的引用。
     */
    static MessageCenter& getInstance();

    /**
     * @brief 获取或创建一个话题。
     * @param name 话题名称。
     * @return std::shared_ptr<ITopic> 返回话题的共享指针。
     */
    std::shared_ptr<ITopic> getTopic(const std::string& name);

    /**
     * @brief 添加一个新话题到消息中心。
     * @param topic 要添加的话题的共享指针。
     */
    void addTopic(std::shared_ptr<ITopic> topic);

private:
    MessageCenter();
    ~MessageCenter();
    MessageCenter(const MessageCenter&) = delete;
    MessageCenter& operator=(const MessageCenter&) = delete;

    std::vector<std::shared_ptr<ITopic>> topics_;
    osMutexId_t center_mutex_; // 使用 osMutexId_t
};

/**
 * @class ITopic
 * @brief 话题的基类接口。
 */
class ITopic {
public:
    virtual ~ITopic() = default;
    virtual const std::string& getName() const = 0;
    virtual void addSubscriber(std::shared_ptr<ISubscriber> sub) = 0;
    virtual void publish(const void* data) = 0;
};

/**
 * @class ISubscriber
 * @brief 订阅者的基类接口。
 */
class ISubscriber {
public:
    virtual ~ISubscriber() = default;
    virtual void push(const void* data) = 0;
};
