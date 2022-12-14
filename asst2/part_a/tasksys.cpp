#include "tasksys.h"
#include <iostream>
IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char *TaskSystemSerial::name() { return "Serial"; }

TaskSystemSerial::TaskSystemSerial(int num_threads)
    : ITaskSystem(num_threads) {}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable *runnable, int num_total_tasks) {
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable *runnable,
                                          int num_total_tasks,
                                          const std::vector<TaskID> &deps) {
  return 0;
}

void TaskSystemSerial::sync() { return; }

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelSpawn::name() {
  return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads)
    : ITaskSystem(num_threads), numThreads_(num_threads) {}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::work(IRunnable *runnable, int num_total_tasks,
                                   std::atomic_int *curTask) {
  int taskNo = 0;
  while (true) {
    taskNo = (*curTask)++;
    if (taskNo >= num_total_tasks) {
      break;
    }
    runnable->runTask(taskNo, num_total_tasks);
  }
}

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks) {
  std::atomic_int curTask(0);
  std::vector<std::thread> workers(numThreads_);
  for (int i = 0; i < numThreads_; ++i) {
    workers[i] = std::thread(&TaskSystemParallelSpawn::work, this, runnable,
                             num_total_tasks, &curTask);
  }
  for (auto &worker : workers) {
    worker.join();
  }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(
    IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) {
  return 0;
}

void TaskSystemParallelSpawn::sync() { return; }

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSpinning::name() {
  return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(
    int num_threads)
    : ITaskSystem(num_threads), numThreads_(num_threads),
      workers_(num_threads) {
  for (int i = 0; i < num_threads; ++i) {
    workers_[i] =
        std::thread(&TaskSystemParallelThreadPoolSpinning::work, this);
  }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
  killed = true;
  for (auto &worker : workers_) {
    worker.join();
  }
}

void TaskSystemParallelThreadPoolSpinning::work() {
  int taskNo = 0;
  while (!killed) {
    // No task, keep spinning
    state_.mut_.lock();
    if (state_.totalTasks_ == 0) {
      state_.mut_.unlock();
      continue;
    }

    taskNo = state_.curTask_++;
    state_.mut_.unlock();
    if (taskNo < state_.totalTasks_) {
      state_.runnable_->runTask(taskNo, state_.totalTasks_);
      state_.mut_.lock();
      state_.doneTasks_++;
      state_.mut_.unlock();
    }
  }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable,
                                               int num_total_tasks) {
  state_.mut_.lock();
  state_.totalTasks_ = num_total_tasks;
  state_.curTask_ = 0;
  state_.doneTasks_ = 0;
  state_.runnable_ = runnable;
  state_.mut_.unlock();

  bool allDone = false;
  while (!allDone) {
    state_.mut_.lock();
    allDone = state_.doneTasks_ >= state_.totalTasks_;
    state_.mut_.unlock();
  }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(
    IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) {
  return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() { return; }

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSleeping::name() {
  return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(
    int num_threads)
    : ITaskSystem(num_threads), numThreads_(num_threads),
      workers_(num_threads) {
  for (int i = 0; i < num_threads; ++i) {
    workers_[i] =
        std::thread(&TaskSystemParallelThreadPoolSleeping::work, this);
  }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
  killed = true;
  cv_.notify_all();
  for (auto &worker : workers_) {
    worker.join();
  }
}

void TaskSystemParallelThreadPoolSleeping::work() {
  int taskNo = 0;
  while (!killed) {
    // No task, keep sleeping
    if (state_.totalTasks_ == 0) {
      std::unique_lock<std::mutex> ulk(cvmut_);
      cv_.wait(ulk);
      ulk.unlock();
    }

    state_.mut_.lock();
    taskNo = state_.curTask_++;
    state_.mut_.unlock();
    if (taskNo < state_.totalTasks_) {
      state_.runnable_->runTask(taskNo, state_.totalTasks_);
      state_.mut_.lock();
      state_.doneTasks_++;
      state_.mut_.unlock();
    }

    if (state_.doneTasks_ >= state_.totalTasks_) {
      cv_.notify_all();
      std::this_thread.yield();
    }
  }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable,
                                               int num_total_tasks) {
  state_.mut_.lock();
  state_.totalTasks_ = num_total_tasks;
  state_.curTask_ = 0;
  state_.doneTasks_ = 0;
  state_.runnable_ = runnable;
  state_.mut_.unlock();

  bool allDone = false;
  while (!allDone) {
    cv_.notify_all();

    std::unique_lock<std::mutex> ulk(cvmut_);
    cv_.wait(ulk);
    allDone = state_.doneTasks_ >= state_.totalTasks_;
    ulk.unlock();
  }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(
    IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) {

  //
  // TODO: CS149 students will implement this method in Part B.
  //

  return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

  //
  // TODO: CS149 students will modify the implementation of this method in
  // Part B.
  //

  return;
}
