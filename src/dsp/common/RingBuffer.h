#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace strata::dsp {

/**
 * Lock-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * Safe for concurrent push from real-time audio thread and pop from UI/analysis thread.
 */
template <typename T>
class RingBuffer final {
public:
    explicit RingBuffer(size_t capacity = 4096) {
        resize(capacity);
    }

    void resize(size_t capacity) {
        // Enforce power of two for fast bitwise masking
        size_t size = 1;
        while (size < capacity) {
            size <<= 1;
        }
        buffer_.assign(size, T{});
        mask_ = size - 1;
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

    void clear() noexcept {
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

    [[nodiscard]] size_t getCapacity() const noexcept {
        return buffer_.size();
    }

    [[nodiscard]] size_t getAvailableForRead() const noexcept {
        const size_t w = writeIndex_.load(std::memory_order_acquire);
        const size_t r = readIndex_.load(std::memory_order_relaxed);
        return (w >= r) ? (w - r) : (buffer_.size() - (r - w));
    }

    [[nodiscard]] size_t getAvailableForWrite() const noexcept {
        return (buffer_.size() - 1) - getAvailableForRead();
    }

    bool push(const T& item) noexcept {
        const size_t w = writeIndex_.load(std::memory_order_relaxed);
        const size_t nextW = (w + 1) & mask_;
        if (nextW == readIndex_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        buffer_[w] = item;
        writeIndex_.store(nextW, std::memory_order_release);
        return true;
    }

    size_t write(const T* data, size_t count) noexcept {
        size_t written = 0;
        while (written < count) {
            if (!push(data[written])) {
                break;
            }
            ++written;
        }
        return written;
    }

    bool pop(T& item) noexcept {
        const size_t r = readIndex_.load(std::memory_order_relaxed);
        if (r == writeIndex_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        item = buffer_[r];
        readIndex_.store((r + 1) & mask_, std::memory_order_release);
        return true;
    }

    size_t read(T* destination, size_t count) noexcept {
        size_t readCount = 0;
        while (readCount < count) {
            if (!pop(destination[readCount])) {
                break;
            }
            ++readCount;
        }
        return readCount;
    }

private:
    std::vector<T> buffer_;
    size_t mask_{0};
    alignas(64) std::atomic<size_t> writeIndex_{0};
    alignas(64) std::atomic<size_t> readIndex_{0};
};

} // namespace strata::dsp
