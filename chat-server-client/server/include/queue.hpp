#ifndef __COUNCURRENT_QUEUE_H
#define __COUNCURRENT_QUEUE_H


#include <stdexcept>
#include <string>
#include <cstring>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <unistd.h>
#include <sys/eventfd.h>


namespace concurrent {

/*
 * Represents Queue exception.
 */
class QueueException: public std::runtime_error {
public:
    QueueException(const std::string& what_arg):
        std::runtime_error(what_arg)
    { }
};

/*
 * Represents a concurrent queue with a notification feature.
 * Non-copyable.
 * Thread-safe.
 */
template<typename T>
class Queue {
public:
    Queue()
    {
        // creates an event file descriptor for notifications. See man eventfd.
        event_fd = eventfd(0, EFD_SEMAPHORE);
        if (event_fd == -1) {
            throw QueueException(std::string("eventfd error: ") + std::strerror(errno));
        }
    }

    Queue(const Queue&) = delete;

    Queue& operator=(const Queue&) = delete;

   ~Queue()
    {
        close(event_fd);
    }

    /*
     * Pushes an object to the queue. Notify the waiters of data availability
     * (selectors or threads called wait_pop method)
     */
    void push(const T& value)
    {
        std::lock_guard<std::mutex> lk(mx);
        data_queue.push(value);

        increment_event_counter();
        data_cond.notify_one();
    }

    /*
     * Assign a poped data to dst reference. If the queue is empty returns false, dst is untouched.
     */
    bool try_pop(T& dst)
    {
        std::lock_guard<std::mutex> lk(mx);
        if (data_queue.empty()) {
            return false;
        }
        dst = data_queue.front();
        data_queue.pop();

        decrement_event_counter();

        return true;
    }

    /*
     * Assign a poped data to dst reference. Blocks if the queue is empty untill any data pushed.
     */
    void wait_pop(T& dst)
    {
        std::unique_lock<std::mutex> lk(mx);
        data_cond.wait(lk, [this] {
            return !data_queue.empty();
        });

        dst = data_queue.front();
        data_queue.pop();

        decrement_event_counter();
    }

    /*
     * Returns shared pointer to a poped data. If the queue is empty returns null pointer.
     */
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mx);
        if(data_queue.empty())
            return std::shared_ptr<T>();

        std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();

        decrement_event_counter();

        return res;
    }

    /*
     * Returns a shred pointer to a poped data. Blocks if the queue is empty untill any data pushed.
     */
    std::shared_ptr<T> wait_pop()
    {
        std::unique_lock<std::mutex> lk(mx);
        data_cond.wait(lk, [this] {
            return !data_queue.empty();
        });

        std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();

        decrement_event_counter();

        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mx);
        return data_queue.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lk(mx);
        return data_queue.size();
    }

    /*
     * Returns an event file descriptor, that is available for reading
     * when the queue contains any data. Can be used by selectors (select, poll, epoll).
     */
    int get_eventfd() const
    {
        std::lock_guard<std::mutex> lk(mx);
        return event_fd;
    }

private:
    int event_fd;                       // event file descriptor for selector notification (select, poll, epoll)
    mutable std::mutex mx;              // mutex for thread-safe support
    std::condition_variable data_cond;  // condition variable for wait_pop methods notifications

    std::queue<T> data_queue;

    // see man eventfd
    void increment_event_counter()
    {
        uint64_t cnt = 1;
        write(event_fd, &cnt, sizeof(uint64_t));
    }

    void decrement_event_counter()
    {
        uint64_t cnt;
        read(event_fd, &cnt, sizeof(uint64_t));
    }
};


} // namespace concurrent


#endif // __COUNCURRENT_QUEUE_H