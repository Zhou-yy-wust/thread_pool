//
// Created by blair on 2024/8/23.
//

#ifndef AUTOTHREAD_H
#define AUTOTHREAD_H

#include <thread>

namespace tp {
    struct join{};
    struct detach{};

    template <typename T>
    class AutoThread{};

    template <>
    class AutoThread<join> {
        std::thread t_;
    public:
        explicit AutoThread(std::thread&& t) noexcept : t_(std::move(t)){}
        AutoThread(AutoThread &&other) noexcept : t_(std::move(other.t_)){}
        AutoThread(const AutoThread&) = delete;
        AutoThread& operator=(const AutoThread&) = delete;
        AutoThread& operator=(AutoThread&& other) noexcept
        {
            if(this != &other)
            {
                t_ = std::move(other.t_);
            }
            return *this;
        }
        ~AutoThread(){if(t_.joinable()) t_.join();}

        using id = std::thread::id;
        [[nodiscard]] id get_id() const {return  t_.get_id();}

    };

    template <>
    class AutoThread<detach> {
        std::thread t_;
    public:
        explicit AutoThread(std::thread&& t) noexcept : t_(std::move(t)){}
        AutoThread(AutoThread &&other) noexcept : t_(std::move(other.t_)){}
        AutoThread(const AutoThread&) = delete;
        AutoThread& operator=(const AutoThread&) = delete;
        AutoThread& operator=(AutoThread&& other) noexcept
        {
            if(this != &other)
            {
                t_ = std::move(other.t_);
            }
            return *this;
        }
        ~AutoThread(){if(t_.joinable()) t_.detach();}

        using id = std::thread::id;
        [[nodiscard]] id get_id() const {return  t_.get_id();}

    };
}

#endif //AUTOTHREAD_H
