#include "message_center.hpp"
#include <algorithm>

// MessageCenter 构造函数，创建互斥锁
MessageCenter::MessageCenter() {
    center_mutex_ = osMutexNew(NULL);
}

// MessageCenter 析构函数，删除互斥锁
MessageCenter::~MessageCenter() {
    osMutexDelete(center_mutex_);
}

MessageCenter& MessageCenter::getInstance() {
    static MessageCenter instance;
    return instance;
}

std::shared_ptr<ITopic> MessageCenter::getTopic(const std::string& name) {
    osMutexAcquire(center_mutex_, osWaitForever);
    auto it = std::find_if(topics_.begin(), topics_.end(),
                           [&name](const std::shared_ptr<ITopic>& topic) {
                               return topic->getName() == name;
                           });
    
    std::shared_ptr<ITopic> found_topic = (it != topics_.end()) ? *it : nullptr;
    osMutexRelease(center_mutex_);
    return found_topic;
}

void MessageCenter::addTopic(std::shared_ptr<ITopic> topic) {
    osMutexAcquire(center_mutex_, osWaitForever);
    if (topic) {
        topics_.push_back(topic);
    }
    osMutexRelease(center_mutex_);
}

// ITopic 中的虚函数默认实现
void ITopic::addSubscriber(std::shared_ptr<ISubscriber> sub) {
    // 这个方法将在派生类中实现
}

void ITopic::publish(const void* data) {
    // 这个方法将在派生类中实现
}
