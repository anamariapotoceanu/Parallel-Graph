# Thread Pool Graph Traversal

## Objectives
- Learn how to design and implement parallel programs
- Gain skills in using synchronization primitives for parallel programs
- Understand the POSIX threading and synchronization API
- Gain insight into the differences between serial and parallel programs

## Statement
Implement a generic thread pool and use it to traverse a graph, computing the sum of the elements contained in the nodes. You will be provided with a serial implementation of the graph traversal and most of the data structures needed to implement the thread pool. Your task is to write the thread pool routines and then use the thread pool to traverse the graph.

## Implementation

### Thread Pool Description
A thread pool contains a specified number of active threads that wait for tasks. The threads are created upon the pool's initialization and continuously poll the task queue for available tasks. 

You must implement the following functions, marked with TODOs in `src/os_threadpool.c`:
- `enqueue_task()`: Enqueue a task to the shared task queue using synchronization.
- `dequeue_task()`: Dequeue a task from the shared task queue using synchronization.
- `wait_for_completion()`: Wait for all worker threads to finish using synchronization.
- `create_threadpool()`: Create a new thread pool.
- `destroy_threadpool()`: Destroy a thread pool, assuming all threads have been joined.

Update the `os_threadpool_t` structure in `src/os_threadpool.h` with the necessary synchronization components. 

Define a condition for the threads to stop polling the task queue once the graph has been fully traversed, specifically in the `wait_for_completion()` function. A recommended approach is to check when no threads have any work left.


## Data Structures
- **Graph**: Internally represented by the `os_graph_t` structure (see `src/os_graph.h`).
- **List**: Internally represented by the `os_queue_t` structure (see `src/os_list.h`). This list will be used to implement the task queue.
- **Thread Pool**: Internally represented by the `os_threadpool_t` structure (see `src/os_threadpool.h`). The thread pool contains information about the task queue and the threads.


