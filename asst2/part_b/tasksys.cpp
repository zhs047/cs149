#include "tasksys.h"

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

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
    : ITaskSystem(num_threads), workers_(num_threads) {
  for (int i = 0; i < num_threads; ++i) {
    workers_[i] =
        std::thread(&TaskSystemParallelThreadPoolSleeping::work, this);
  }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
  killed_ = true;
  masterCv_.notify_all();
  workerCv_.notify_all();
  for (auto &worker : workers_) {
    worker.join();
  }
}

void TaskSystemParallelThreadPoolSleeping::work() {
  while (!killed_) {
    dataLock_.lock();

    if (ready_.empty()) {
      dataLock_.unlock();
      masterCv_.notify_all();
      std::unique_lock<std::mutex> ulk(workerLock_);
      workerCv_.wait(ulk);
      ulk.unlock();
      continue;
    }

    auto [batchId, totalTasks, runnable, taskNo] = ready_.front();
    ready_.pop();
    dataLock_.unlock();

    runnable->runTask(taskNo, totalTasks);
    dataLock_.lock();
    ++progress_[batchId];
    dataLock_.unlock();

    if (progress_[batchId] >= totalTasks) {
      checkPending(batchId);
    }
  }
}

void TaskSystemParallelThreadPoolSleeping::checkPending(TaskID doneId) {
  std::vector<TaskID> toReady{};
  dataLock_.lock();

  batchDone_.insert(doneId);
  for (auto &[id, dep] : dependency_) {
    dep.erase(doneId);
    if (dep.empty()) {
      toReady.push_back(id);
    }
  }

  for (const TaskID id : toReady) {
    dependency_.erase(id);
    if (pending_.count(id) != 0) {
      auto [batchId, totalTasks, runnable, ignore] = pending_.at(id);
      for (int i = 0; i < totalTasks; ++i) {
        ready_.emplace(batchId, totalTasks, runnable, i);
      }
      pending_.erase(id);
    }
  }
  dataLock_.unlock();
  masterCv_.notify_all();
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable,
                                               int num_total_tasks) {
  runAsyncWithDeps(runnable, num_total_tasks, {});
  sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(
    IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) {
  dataLock_.lock();
  TaskID ret = nextBatchId_++;

  std::unordered_set<TaskID> tmpDep{};

  for (const TaskID batchId : deps) {
    if (batchDone_.count(batchId) == 0) {
      tmpDep.insert(batchId);
    }
  }

  if (tmpDep.empty()) {
    for (int i = 0; i < num_total_tasks; ++i) {
      ready_.emplace(ret, num_total_tasks, runnable, i);
    }
  } else {
    pending_.emplace(ret, Task(ret, num_total_tasks, runnable, -1));
    dependency_.emplace(ret, tmpDep);
  }
  progress_.emplace(ret, 0);

  dataLock_.unlock();
  workerCv_.notify_all();
  return ret;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
  bool allDone = false;
  while (!allDone) {
    dataLock_.lock();
    allDone = batchDone_.size() == progress_.size();
    dataLock_.unlock();
    workerCv_.notify_all();
    std::unique_lock<std::mutex> ulk(masterLock_);
    masterCv_.wait(ulk);
    ulk.unlock();
  }
  workerCv_.notify_all();
}

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
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
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
    : ITaskSystem(num_threads) {}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks) {
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(
    IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) {
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
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
    : ITaskSystem(num_threads) {}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable,
                                               int num_total_tasks) {
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(
    IRunnable *runnable, int num_total_tasks, const std::vector<TaskID> &deps) {
  for (int i = 0; i < num_total_tasks; i++) {
    runnable->runTask(i, num_total_tasks);
  }
  return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() { return; }
