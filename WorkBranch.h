//
// Created by blair on 2024/8/23.
//

#ifndef WORKBRANCH_H
#define WORKBRANCH_H

#include <condition_variable>

#include "AutoThread.h"
#include "BlockingQueue.h"
#include "Utility.h"
#include <map>
#include <functional>
#include <stdexcept>
#include <iostream>

namespace tp {
    class WorkBranch {
        using worker = AutoThread<detach>;
        using worker_map = std::map<worker::id, worker>;

    private:
        worker_map workers_{};
        BlockingQueue<std::function<void()>> tasks_{};

        std::mutex mtx_;
        std::condition_variable thread_cv_;  // 通知，有任务到来
        std::condition_variable task_done_;  // 通知，有空闲进程

        std::size_t decline_ = 0;  // 需要减少进程的数量
        std::size_t task_done_workers_ = 0;  // 空闲进程数量
        bool is_waiting_ = false;   // 线程池是否正在等待所有任务完成。
        bool destructing_ = false;  // 线程池是否正在被析构。

    public:
        explicit WorkBranch(int wks=1) {
            for(int i = 0; i <= std::max(wks, 1); ++i)
                add_worker();
        }

        WorkBranch(const WorkBranch&) = delete;
        WorkBranch(WorkBranch&&) = delete;
        ~WorkBranch() {
            std::unique_lock<std::mutex> lock(mtx_);
            decline_ = workers_.size();
            destructing_ = true;
            thread_cv_.wait(lock, [this](){return !decline_;});
        }

    public:
        void add_worker() {
            std::lock_guard<std::mutex> lock(mtx_);
            std::thread t(&WorkBranch::mission, this);
            workers_.emplace(t.get_id(), std::move(t));
        }

        void del_worker() {
            std::lock_guard<std::mutex> lock(mtx_);
            if(workers_.empty()) {
                throw std::runtime_error("workspace: No worker in workbranch to delete");
            }
            ++decline_;
        }

        bool wait_tasks(unsigned timeout=1) {
            bool res;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                is_waiting_ = true;
                res = task_done_.wait_for(lock, std::chrono::microseconds(timeout), [this]() {
                    return task_done_workers_ >= workers_.size();
                });
                task_done_workers_ = 0;
                is_waiting_ = false;
            }
            thread_cv_.notify_all();
            return res;
        }
        std::size_t num_workers() {
            std::lock_guard<std::mutex> lock(mtx_);
            return workers_.size();
        }
        std::size_t num_tasks() {
            std::lock_guard<std::mutex> lock(mtx_);
            return tasks_.size();
        }
    public:
        // enable_if 限制模板的实例化条件
        template <
            typename T = normal,
            typename F,
            typename R = result_of<F>,
            typename DR = std::enable_if_t<std::is_void_v<R>>
        >  // 当且仅当 R是void、T是normal时被实例化
        auto submit(F &&task) -> std::enable_if_t<std::is_same_v<T, normal>> {
            tasks_.push_back(make_task_wrapper(std::forward<F>(task)));
        }

        template<
            typename T,
            typename F,
            typename R = result_of<F>,
            typename DR = std::enable_if_t<std::is_void_v<R>>
            > // 当且仅当 R是void、T是urgent时被实例化
        auto submit(F &&task) -> std::enable_if_t<std::is_same_v<T, urgent>> {
            tasks_.push_front(make_task_wrapper(std::forward<F>(task)));
        }

        template <
            typename T,
            typename F,
            typename ...Fs
            > // 当且仅当 R是void、T是sequence时被实例化
        auto submit(F&& task, Fs&& ...tasks) -> std::enable_if_t<std::is_same_v<T, sequence>> {
            tasks_.push_back(
                make_task_wrapper(
                    [=] {this->rexec(task, tasks...);}
                    ));
        }

        template <
            typename T = normal,
            typename F,
            typename R = result_of<F>,
            typename DR = std::enable_if_t<!std::is_void_v<R>>
        > // 当且仅当R不是void、T是normal时被实例化
        auto submit(F&& task, std::enable_if<std::is_same_v<T, normal>, normal> = {}) -> std::future<R> {
            std::function<R()> exec(std::forward<F>(task));
            std::shared_ptr<std::promise<R>> take_promise = std::make_shared<std::promise<R>>();
            tasks_.push_back(
                make_task_wrapper(
                    [exec, take_promise]() { take_promise->set_value(exec());}
                    ));
            return take_promise->get_future();
        }

        template <
            typename T,
            typename F,
            typename R = result_of<F>,
            typename DR = std::enable_if_t<!std::is_void_v<R>>
        > // 当且仅当R不是void、T是urgent时被实例化
        auto submit(F&& task, std::enable_if<std::is_same_v<T, urgent>, urgent> = {}) -> std::future<R> {
            std::function<R()> exec(std::forward<F>(task));
            std::shared_ptr<std::promise<R>> take_promise = std::make_shared<std::promise<R>>();
            tasks_.push_front(make_task_wrapper(
                [exec, take_promise]() {take_promise->set_value(exec());}
                ));
            return take_promise->get_future();
        }

    private:
        void mission() {   // 每个线程任务，要么执行任务队列的任务，要么阻塞，要么析构
            std::function<void()> task;
            while(true) {
                if(decline_ <= 0 && tasks_.try_pop(task))  // 尝试取出任务，并执行，但不阻塞
                    task();
                else if(decline_ > 0) {  // 析构
                    std::lock_guard<std::mutex> lock(mtx_);
                    if(decline_ > 0 && --decline_) {  // 双重检查
                        workers_.erase(std::this_thread::get_id());
                        if(is_waiting_)
                            task_done_.notify_one();
                        if(destructing_)
                            thread_cv_.notify_one();
                        return;
                    }
                }
                else if(is_waiting_)
                    {  // 阻塞
                        std::unique_lock<std::mutex> lock(mtx_);
                        ++task_done_workers_;
                        task_done_.notify_one();
                        thread_cv_.wait(lock);
                    }
                else
                    std::this_thread::yield(); // 让出时间片

            }
        }

        template <typename F>
        static std::function<void()> make_task_wrapper(F &&task) {
            return [task]() {
                try {
                    task();
                } catch (const std::exception& ex) {
                    std::cerr<<"workspace: worker["<< std::this_thread::get_id()<<"] caught exception:\n  what(): "<<ex.what()<<'\n'<<std::flush;
                } catch (...) {
                    std::cerr<<"workspace: worker["<< std::this_thread::get_id()<<"] caught unknown exception\n"<<std::flush;
                }
            };
        }

        template <typename F>
        void rexec(F&& task){task();}

        template <typename F, typename ...Fs>
        void rexec(F&& first, Fs ...funcs) {
            first();
            rexec(std::forward<Fs>(funcs)...);
        }

    };

}

#endif //WORKBRANCH_H
