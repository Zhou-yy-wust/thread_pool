//
// Created by blair on 2024/8/25.
//

#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <functional>
#include <utility>

#include "AutoThread.h"
#include "WorkBranch.h"

namespace tp {
    class Supervisor {
        using tick_callback_t = std::function<void()>;
    private:
        bool stop_ = false;

        std::size_t wmin_ = 0;
        std::size_t wmax_ = 0;
        unsigned timeout_ = 0;
        const unsigned tval_ = 0;

        tick_callback_t tick_cb_ = {};
        AutoThread<join> worker_;
        std::vector<WorkBranch*> branches_;
        std::condition_variable thrd_cv_;
        std::mutex spv_lok_;
    public:
        Supervisor(int min_wokrs, int max_wokrs, unsigned time_interval = 500)
            : wmin_(min_wokrs)
            , wmax_(max_wokrs)
            , timeout_(time_interval)
            , tval_(time_interval)
            , worker_(std::thread(&Supervisor::mission, this)) {
            static_assert(min_wokrs >= 0 && max_wokrs > 0 && max_wokrs > min_wokrs);
        }

        Supervisor(const Supervisor&) = delete;
        Supervisor& operator=(const Supervisor&) = delete;
        ~Supervisor() {
            std::lock_guard<std::mutex> lock(spv_lok_);
            stop_ = true;
            thrd_cv_.notify_one();
        }

    public:
        void supervise(WorkBranch& wbr) {
            std::lock_guard<std::mutex> lock(spv_lok_);
            branches_.emplace_back(&wbr);
        }

        void proceed() {
            {
                std::lock_guard<std::mutex> lock(spv_lok_);
                timeout_ = tval_;
            }
            thrd_cv_.notify_one();
        }

        void set_tick_tb(tick_callback_t cb) {
            tick_cb_ = std::move(cb);
        }

    private:
        void mission() {
            while(!stop_) {
                try {
                    {
                        std::unique_lock<std::mutex> lock(spv_lok_);
                        for(auto pbr: branches_) {
                            auto tknums = pbr->num_tasks();
                            auto wknums = pbr->num_workers();
                            if(tknums) {
                                static_assert(wknums <= wmax_);
                                std::size_t nums = std::min(wmax_-wknums, tknums-wknums);
                                for(std::size_t i = 0; i<nums; i++)
                                    pbr->add_worker();
                            }
                            else if (wknums > wmin_)
                                pbr->del_worker();
                        }
                        if(!stop_)
                            thrd_cv_.wait_for(lock, std::chrono::milliseconds(timeout_));
                    }
                    tick_cb_();
                } catch (const std::exception& ex) {
                    std::cerr<<"workspace: supervisor["<< std::this_thread::get_id()<<"] caught exception:\n  \
                what(): "<<ex.what()<<'\n'<<std::flush;
                }
            }
        }
    };
}

#endif //SUPERVISOR_H
