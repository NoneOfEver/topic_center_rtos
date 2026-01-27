#pragma once

#include "cmsis_os2.h"

/*
 * Simple uORB-lite notifier
 * publish -> wake worker
 */

class UORBNotifier {
public:
    explicit UORBNotifier(osEventFlagsId_t evt, uint32_t mask)
        : evt_(evt), mask_(mask)
    {}

    inline void notify()
    {
        osEventFlagsSet(evt_, mask_);
    }

private:
    osEventFlagsId_t evt_;
    uint32_t mask_;
};
