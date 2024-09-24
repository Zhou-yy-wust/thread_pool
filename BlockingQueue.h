//
// Created by blair on 2024/8/23.
//

#ifndef BLOCKING_QUEUE_CPP
#define BLOCKING_QUEUE_CPP

#include <atomic>
#include <deque>
#include <mutex>

namespace tp {

    template <typename T>
    class BlockingQueue {
    public:
        using size_type = typename std::deque<T>::size_type;
        BlockingQueue() = default;
        BlockingQueue(const BlockingQueue&) = delete;
        BlockingQueue(BlockingQueue&&) = default;
        BlockingQueue& operator=(const BlockingQueue&) = delete;
        BlockingQueue& operator=(BlockingQueue&&) = default;

        void push_back(const T& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.push_back(v);
        }

        void push_back(T&& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.push_back(std::move(v));
        }

        void emplace_back(const T& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.emplace_back(v);
        }

        void emplace_back(T&& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.emplace_back(std::move(v));
        }

        void push_front(const T& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.push_front(v);
        }

        void push_front(T&& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.push_front(std::move(v));
        }

        void emplace_front(const T& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.emplace_front(v);
        }

        void emplace_front(T&& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_.emplace_front(std::move(v));
        }

        bool try_pop(T& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            if(!data_.empty()) {
                v = std::move(data_.front());
                data_.pop_front();
                return true;
            }
            return false;
        }

        size_type size() const {
            std::lock_guard<std::mutex> lock(mtx_);
            return data_.size();
        }

    private:
        mutable std::mutex mtx_;
        std::deque<T> data_;
    };

}

#endif
