#pragma once

#include <queue>

#include <atomic>
#include <thread>
#include <future>

// For processing image rows in parallel using a limited number of threads.
class RowQueue {
private:
    std::queue<uint> rqueue;
    mutable std::mutex mut;
public:
    std::atomic<uint> rowsProcessed;
    struct extract_pair { uint row; bool success; };

    RowQueue(): rqueue(), mut(), rowsProcessed(0U) {};
    
    // Assume that threads will only ever pop from the queue, and 
    // that the job of delegating tasks to threads is single-threaded.
    inline void push(const uint &val) { rqueue.push(val); }
    
    extract_pair pop() {
        std::unique_lock<std::mutex> lock(mut);
        extract_pair pair;
        // No more rows to process in the queue, so terminate early.
        if (rqueue.empty()) { 
            pair.success = false; 
            return pair; 
        }

        pair.row = rqueue.front();
        rqueue.pop();
        pair.success = true;
        return pair;
    }

    inline bool empty() const { return rqueue.empty(); }
};