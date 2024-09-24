//
// Created by blair on 2024/8/23.
//

#ifndef UTILITY_H
#define UTILITY_H

#include <type_traits>
#include <deque>
#include <future>
#include <vector>


#if defined(_MSC_VER)

#if _MSVC_LANG >= 201703L
template <typename F, typename ...Args>
using result_of = std::invoke_result_t<F, Args...>;
#else
template <typename F, typename  ...Args>
using result_of = std::result_of<F(Args...)>::type;
#endif

#else
#if __cplusplus >= 201703L
template <typename F, typename ...Args>
using result_of = std::invoke_result_t<F, Args...>;
#else
template <typename F, typename  ...Args>
using result_of = std::result_of<F(Args...)>::type;
#endif
#endif


struct normal{};
struct urgent{};
struct sequence{};

template <typename T>
class futures {
    std::deque<std::future<T>> futs_;
public:
    using iterator = typename std::deque<std::future<T>>::iterator;

    void wait() {
        for(auto& f:futs_)
            f.wait();
    }

    [[nodiscard]] std::size_t size() const {
        return futs_.size();
    }

    std::vector<T> get() {
        std::vector<T> res;
        for(auto& f: futs_)
            res.emplace_back(f.get());
        return res;
    }

    iterator begin() {return futs_.begin();}
    iterator end() {return futs_.end();}
    void add_back(std::future<T>&& fut) {
        futs_.emplace_back(std::move(fut));
    }
    void add_front(std::future<T>&& fut) {
        futs_.emplace_front(std::move(fut));
    }

    void for_each(const iterator& first, std::function<void(std::future<T>&)> deal) {
        for(auto it=first; it!=end(); ++it) {
            deal(*it);
        }
    }

    void for_each(const iterator& first, const iterator& last, std::function<void(std::future<T>&)> deal) {
        for(auto it=first; it!=last; ++it) {
            deal(*it);
        }
    }

    auto operator[](std::size_t idx) -> std::future<T>& {
        return futs_[idx];
    }

};


#endif //UTILITY_H
