#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>

#include "uorb_notify.hpp"

/*
 * uORB-lite topic
 * Static shared buffer + generation counter
 */

template<typename T>
class UORBTopic {
public:
    constexpr UORBTopic() : generation_(0) {}

    inline void publish(const T& data)
    {
        buffer_ = data;
        generation_.fetch_add(1, std::memory_order_release);
    }

    inline uint32_t generation() const
    {
        return generation_.load(std::memory_order_acquire);
    }

    inline void copy(T& out) const
    {
        out = buffer_;
    }

private:
    T buffer_{};
    std::atomic<uint32_t> generation_;
};

/*
 * Multi-instance topic
 */

template<typename T, int N>
class UORBMultiTopic {
public:
    constexpr UORBMultiTopic() {}

    inline UORBTopic<T>& operator[](int i)
    {
        return topics_[i];
    }

    inline const UORBTopic<T>& operator[](int i) const
    {
        return topics_[i];
    }


private:
    UORBTopic<T> topics_[N];

};

/*
 * Ringbuffer topic (history)
 */

template<typename T, int DEPTH>
class UORBRingTopic {
public:
    constexpr UORBRingTopic() : write_index_(0), generation_(0) {}

    inline void publish(const T& data)
    {
        ring_[write_index_] = data;
        write_index_ = (write_index_ + 1) % DEPTH;
        generation_.fetch_add(1, std::memory_order_release);

        if (notifier_) {
            notifier_->notify();
        }
    }


    inline uint32_t generation() const
    {
        return generation_.load(std::memory_order_acquire);
    }

    inline bool copy(uint32_t gen, T& out) const
    {
        uint32_t current = generation();

        if (gen == current) {
            return false;
        }

        uint32_t delta = current - gen;

        if (delta > DEPTH) {
            // subscriber 太慢，直接给最新
            out = ring_[(write_index_ + DEPTH - 1) % DEPTH];
        } else {
            uint32_t index =
                (write_index_ + DEPTH - delta) % DEPTH;
            out = ring_[index];
        }

        return true;
    }

    inline void register_notifier(UORBNotifier* n)
    {
        notifier_ = n;
    }


private:
    T ring_[DEPTH]{};
    uint32_t write_index_;
    std::atomic<uint32_t> generation_;
    UORBNotifier* notifier_{nullptr};

};

/*
 * Subscription (pull based)
 */

template<typename T>
class UORBSubscription {
public:
    explicit UORBSubscription(UORBTopic<T>& topic)
        : topic_(topic)
    {
        last_generation_ = topic_.generation();
    }

    inline bool updated()
    {
        return topic_.generation() != last_generation_;
    }

    inline bool copy(T& out)
    {
        uint32_t gen = topic_.generation();

        if (gen == last_generation_) {
            return false;
        }

        topic_.copy(out);
        last_generation_ = gen;
        return true;
    }

private:
    UORBTopic<T>& topic_;
    uint32_t last_generation_;
};


template<typename T, int DEPTH>
class UORBRingSub {
public:
    explicit UORBRingSub(UORBRingTopic<T, DEPTH>& topic)
        : topic_(topic)
    {
        last_generation_ = topic_.generation();
    }

    inline bool copy(T& out)
    {
        uint32_t gen = last_generation_;

        if (!topic_.copy(gen, out)) {
            return false;
        }

        last_generation_++;
        return true;
    }

private:
    UORBRingTopic<T, DEPTH>& topic_;
    uint32_t last_generation_;
};
