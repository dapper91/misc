#ifndef __EPOLL_H
#define __EPOLL_H


#include <stdexcept>
#include <string>
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>


namespace io {


using namespace std::placeholders;


/*
 * Represents Epoll exception.
 */
class EpollExcepton: public std::runtime_error {
public:
    EpollExcepton(const std::string& what_arg):
        std::runtime_error(what_arg)
    { }
};


/*
 * Represents a epoll selector. Dispatches epoll events to the added handlers.
 * Non-copyable.
 * Not thread-safe.
 */
class Epoll {
public:
    enum Event {
        IN      = EPOLLIN,
        OUT     = EPOLLOUT,
        ONESHOT = EPOLLONESHOT,
        RDHUP   = EPOLLRDHUP,
        HUP     = EPOLLHUP,
        ERR     = EPOLLERR
    };

    /*
     * Constructor.
     * params:
     *      max_events - maximum file descriptors number to be polled
     */
    Epoll(size_t max_events);

    Epoll(const Epoll&) = delete;

    Epoll& operator=(const Epoll&) = delete;

    /*
     * Starts a epoll event loop. Dispatches events to the added handlers.
     */
    void start();

    /*
     * Stops the epoll loop.
     */
    void stop();

    /*
     * Adds a handler to the epoll event loop.
     * params:
     *      fd          - file descriptor
     *      event_mask  - mask of events to be handled
     *      func        - functional object to be called on a epoll event
     *      data        - additianal data pointer to be passed to the handler as the last parameter
     */
    void add_handler(int fd, int event_mask, std::function<void(int, void*)> func, void* data = nullptr);

    /*
     * Deletes a handler from the epoll event loop by a file descriptor.
     * params:
     *      fd - file descriptor to be deleted
     */
    void del_handler(int fd);

private:
    bool stop_flag = false;
    size_t max_events;
    int epollfd;
    std::unordered_map<int, std::function<void(int)>> handlers;     // handlers map to be called by the events
};


} // namespace io


#endif // __EPOLL_H