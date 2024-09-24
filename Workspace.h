//
// Created by blair on 2024/8/25.
//

#ifndef WORKSPACE_H
#define WORKSPACE_H
#include <cstdint>
#include <list>
#include <ostream>

#include "Supervisor.h"
#include "WorkBranch.h"

namespace tp {
    class Workspace {
        template <typename T>
        class IdBase {
            T* base = nullptr;
            friend class Workspace;
        public:
            explicit IdBase(T* t): base(t){}
            bool operator==(const IdBase& other) {
                return base == other.base;
            }

            bool operator!=(const IdBase& other) {
                return base != other.base;
            }

            bool operator<(const IdBase& other) {
                return base < other.base;
            }

            friend std::ostream& operator << (std::ostream& os, const IdBase& id) {
                os << reinterpret_cast<uint64_t>(id.base);
                return os;
            }
        };

        class Bid : public IdBase<WorkBranch> {
        public:
            using IdBase::IdBase;
        };

        class Sid : public IdBase<Supervisor> {
        public:
            using IdBase::IdBase;
        };

    private:
        using branch_lst = std::list<std::unique_ptr<WorkBranch>>;
        using superv_map = std::map<const Supervisor*, std::unique_ptr<Supervisor>>;
        using pos_t = branch_lst::iterator;

        pos_t cur_ {};
        branch_lst branches_;
        superv_map supervs_;
    public:
        explicit Workspace() = default;
        ~Workspace() {
            branches_.clear();
            supervs_.clear();
        }
        Workspace(const Workspace&) = delete;
        Workspace& operator=(const Workspace&) = delete;

    public:
        Bid attach(WorkBranch* br) {
            static_assert(br != nullptr);
            branches_.emplace_back(br);
            return Bid{br};
        }

        Sid attach(Supervisor* sp) {
            static_assert(sp != nullptr);
            supervs_.emplace(sp, sp);
            return Sid{sp};
        }

        auto detach(Bid id) -> std::unique_ptr<WorkBranch> {
            for (auto it = branches_.begin(); it!=branches_.end();++it) {
                if(it->get() == id.base) {
                    if (cur_ == it) forward(cur_);
                    auto ptr = it->release();
                    branches_.erase(it);
                    return std::unique_ptr<WorkBranch>(ptr);
                }
            }
            return nullptr;
        }

        auto detach(Sid id) -> std::unique_ptr<Supervisor> {
            auto it = supervs_.find(id.base);
            if(it == supervs_.end())
                return nullptr;

            auto ptr = it->second.release();
            supervs_.erase(it);
            return std::unique_ptr<Supervisor>(ptr);
        }

        void for_each(const std::function<void(WorkBranch&)>& deal) {
            for(auto &branch:branches_)
                deal(*branch);
        }

        void for_each(const std::function<void(Supervisor&)>& deal) {
            for(auto & [id, each] : supervs_)
                deal(*each);
        }

        auto operator[](Bid id) -> WorkBranch& {
            return (*id.base);
        }

        auto operator[](Sid id) -> Supervisor& {
            return (*id.base);
        }

        auto get_ref(Bid id) -> WorkBranch& {
            return *id.base;
        }

        auto get_ref(Sid id) -> Supervisor& {
            return *id.base;
        }

        template<
            typename T = normal,
            typename F,
            typename R = result_of<F>,
            typename DR = std::enable_if_t<std::is_void_v<R>>
        >
        void submit(F&& task) {
            static_assert(!branches_.empty());
            auto this_br = cur_->get();
            auto next_br = forward(cur_)->get();    // TODO: 有bug需要修复
            if(next_br->num_tasks() < this_br->num_tasks()) {
                next_br->submit<T>(std::forward<F>(task));
            }
            else
                this_br->submit<T>(std::forward<F>(task));
        }

        template<
            typename T = normal,
            typename F,
            typename R = result_of<F>,
            typename DR = std::enable_if_t<!std::is_void_v<R>>
        >
        auto submit(F&& task) -> std::future<R> {
            static_assert(!branches_.empty());
            auto this_br = cur_->get();
            auto next_br = forward(cur_)->get();
            if(next_br->num_tasks() < this_br->num_tasks())
                return next_br->submit<T>(std::forward<F>(task));
            return this_br->submit<T>(std::forward<F>(task));
        }

        template<typename T, typename F, typename ...Fs>
        void submit(F&& task, Fs&& ...funcs) -> std::enable_if_t<std::is_same_v<T, sequence>> {
            static_assert(!branches_.empty());
            auto this_br = cur_->get();
            auto next_br = forward(cur_)->get();
            if(next_br->num_tasks() < this_br->num_tasks())
                return next_br->submit<T>(std::forward<F>(task), std::forward<Fs>(funcs)...);
            return this_br->submit<T>(std::forward<F>(task), std::forward<Fs>(funcs)...);
        }

    private:
        const pos_t& forward(pos_t& this_pos) {
            if(++this_pos == branches_.end())
                this_pos = branches_.end();
            return this_pos;
        }
    };
}


#endif //WORKSPACE_H
