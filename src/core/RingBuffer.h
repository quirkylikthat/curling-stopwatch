// Fixed-capacity ring buffer (no heap). Index 0 is the NEWEST item.
#pragma once

#include <stddef.h>

namespace curl {

template <typename T, size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0, "RingBuffer capacity must be > 0");

public:
    void push(const T& item) {
        head_ = (head_ + 1) % Capacity;
        items_[head_] = item;
        if (size_ < Capacity) ++size_;
    }

    void clear() { size_ = 0; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    static constexpr size_t capacity() { return Capacity; }

    // 0 = newest, size()-1 = oldest. Caller must check i < size().
    const T& operator[](size_t i) const { return items_[(head_ + Capacity - i) % Capacity]; }

private:
    T items_[Capacity] = {};
    size_t head_ = Capacity - 1;  // so the first push lands in slot 0
    size_t size_ = 0;
};

}  // namespace curl
