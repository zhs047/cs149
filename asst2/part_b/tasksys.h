#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
struct Task {
  TaskID batchId_;
  int totalTasks_;
  IRunnable *runnable_{nullptr};

  int taskNo_;

  Task(TaskID id, int totalTasks, IRunnable *runnable, int taskNo)
      : batchId_(id), totalTasks_(totalTasks), runnable_(runnable),
        taskNo_(taskNo) {}
};

class TaskSystemParallelThreadPoolSleeping : public ITaskSystem {
private:
  int numThreads_;
  std::vector<std::thread> workers_;
  bool killed_{false};

  TaskID nextBatchId_{0};
  std::queue<Task> ready_{};
  std::unordered_map<TaskID, int> progress_{};
  std::unordered_map<TaskID, Task> pending_{};
  std::unordered_set<TaskID> batchDone_{};
  std::unordered_map<TaskID, std::unordered_set<TaskID>> dependency_{};

  std::mutex dataLock_{};

  std::mutex workerLock_{};
  std::condition_variable workerCv_{};
  std::mutex masterLock_{};
  std::condition_variable masterCv_{};

  void work();
  void checkPending(TaskID doneId);

public:
  TaskSystemParallelThreadPoolSleeping(int num_threads);
  ~TaskSystemParallelThreadPoolSleeping();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial : public ITaskSystem {
public:
  TaskSystemSerial(int num_threads);
  ~TaskSystemSerial();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn : public ITaskSystem {
public:
  TaskSystemParallelSpawn(int num_threads);
  ~TaskSystemParallelSpawn();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning : public ITaskSystem {
public:
  TaskSystemParallelThreadPoolSpinning(int num_threads);
  ~TaskSystemParallelThreadPoolSpinning();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

#endif
