#pragma once

#include "uorb_lite.hpp"

/*
 * Publisher wrapper
 */

template<typename T>
class UORBPublisher {
public:
    explicit UORBPublisher(UORBTopic<T>& topic)
        : topic_(topic)
    {}

    inline void publish(const T& data)
    {
        topic_.publish(data);
    }

private:
    UORBTopic<T>& topic_;
};


/*
 * Subscription wrapper
 */

template<typename T>
class UORBSub {
public:
    explicit UORBSub(UORBTopic<T>& topic)
        : sub_(topic)
    {}

    inline bool updated()
    {
        return sub_.updated();
    }

    inline bool copy(T& out)
    {
        return sub_.copy(out);
    }

private:
    UORBSubscription<T> sub_;
};
