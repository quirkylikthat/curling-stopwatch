// Tiny fixed-size FIFO for InputEvents produced by polling (no heap).
// (Interrupt-produced events use a FreeRTOS queue in the HAL instead, because
// that one has to be safe to write from an ISR.)
#pragma once

#include <stddef.h>

#include "Types.h"

namespace curl {

template <size_t Capacity>
class EventFifo {
public:
    bool push(const InputEvent& e) {
        if (count_ == Capacity) return false;  // drop newest when full
        items_[(head_ + count_) % Capacity] = e;
        ++count_;
        return true;
    }

    bool pop(InputEvent& out) {
        if (count_ == 0) return false;
        out = items_[head_];
        head_ = (head_ + 1) % Capacity;
        --count_;
        return true;
    }

    size_t size() const { return count_; }

private:
    InputEvent items_[Capacity] = {};
    size_t head_ = 0;
    size_t count_ = 0;
};

}  // namespace curl
