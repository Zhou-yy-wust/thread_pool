#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include "WorkBranch.h"

using namespace tp;

void example_normal_task(int id) {
    std::cout << "Normal Task " << id << " is running on thread " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Normal Task " << id << " is completed." << std::endl;
}

void example_urgent_task(int id) {
    std::cout << "Urgent Task " << id << " is running on thread " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Urgent Task " << id << " is completed." << std::endl;
}

int example_task_with_return(int id) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return id * 2;  // 返回处理结果
}

int main() {
    WorkBranch pool(3); // 创建一个包含3个工作线程的线程池

    // 提交正常任务
    for (int i = 0; i < 3; ++i) {
        pool.submit([i](){example_normal_task(i);});
    }

    // 提交紧急任务
    for (int i = 0; i < 3; ++i) {
        pool.submit<urgent>([i](){example_urgent_task(i);});
    }

    // 提交返回值任务
    std::vector<std::future<int>> results;
    for (int i = 0; i < 3; ++i) {
        results.push_back(pool.submit(
            [i](){return example_task_with_return(i);})
            );
    }

    // 获取并输出返回值
    for (auto &result : results) {
        std::cout << "Result: " << result.get() << std::endl;  // 获取返回值
    }

    // 等待所有任务完成
    pool.wait_tasks();
    std::cout << "All tasks completed." << std::endl;

    return 0;
}
