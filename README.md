# High-Performance Thread Pool Framework

This project implements a thread pool system based on C++11, supporting dynamic task scheduling and thread management. The task queue can store various callable objects, including regular functions, lambda functions, member functions, and functors.

## Project Highlights

- **Unified `submit` Interface**: Provides a unified task submission interface through SFINAE (Substitution Failure Is Not An Error), supporting urgent, normal, sequential tasks, and tasks with or without return values.
- **Thread Pool State Management**: Implements four states for thread management, including thread deletion, normal execution, waiting for tasks, and yielding CPU time.

## Example Usage

To build and run the example, use the following commands:

```shell
cmake -B build && cmake --build build
./ThreadPool
```

## Reference
For more information, please check the repository: [workspace](https://github.com/CodingHanYa/workspace.git)