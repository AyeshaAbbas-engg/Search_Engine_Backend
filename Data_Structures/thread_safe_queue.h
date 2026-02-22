#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include "queue.h"
#include <mutex>
#include <condition_variable>
#include <string>

class ThreadSafeQueue {
private:
    Queue q;                                  
    mutable std::mutex mtx;                    // Mutable for use in const empty()
    std::condition_variable cv;                // Notifies waiting consumers

public:
    ThreadSafeQueue() = default;

    // Delete copy constructor and assignment to prevent double-locking/double-free issues
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Producer: add a URL to the queue
    void push(const std::string& item) {
        std::unique_lock<std::mutex> lock(mtx);
        q.enqueue(item);
        cv.notify_one();                       // Wake one waiting consumer
    }

    // Consumer: non-blocking attempt to get an item
    bool try_pop(std::string& item) {
        std::unique_lock<std::mutex> lock(mtx);
        if (q.isEmpty()) {
            return false;
        }
        item = q.dequeue();
        return true;
    }

    // Consumer: blocking wait until an item is available
    void wait_and_pop(std::string& item) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.isEmpty(); });
        item = q.dequeue();
    }

    // Check if queue is empty (thread-safe)
    bool empty() const {
        std::unique_lock<std::mutex> lock(mtx);
        return q.isEmpty();
    }

    // Clear all items — useful for reset or shutdown
    void clear() {
        std::unique_lock<std::mutex> lock(mtx);
        while (!q.isEmpty()) {
            q.dequeue();                       // Properly frees nodes
        }
        cv.notify_all();                       // Wake any waiting threads
    }
};

#endif 