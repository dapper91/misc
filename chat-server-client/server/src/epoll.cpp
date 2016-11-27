#include <epoll.h>

#include <string>
#include <cstring>
#include <functional>
#include <sys/epoll.h>


namespace io {


using namespace std::placeholders;


Epoll::Epoll(size_t max_events):
    max_events(max_events)
{
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        throw EpollExcepton(std::string("epoll_create1 error: ") + std::strerror(errno));
    }
}


void Epoll::start()
{
    epoll_event events[max_events];

    while (!stop_flag) {
        int nfds = epoll_wait(epollfd, events, max_events, -1);
        if (nfds == -1) {
            throw EpollExcepton(std::string("epoll_wait error: ") + std::strerror(errno));
        }

        for (size_t n = 0; n < nfds; n++) {
            handlers[events[n].data.fd](events[n].events);
        }
    }
}


void Epoll::stop()
{
    stop_flag = true;
}


void Epoll::add_handler(int fd, int event_mask, std::function<void(int, void*)> func, void* data)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = event_mask;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        throw EpollExcepton(std::string("epoll_ctl error: ") + std::strerror(errno));
    }

    handlers[fd] = std::bind(func, _1, data);
}

void Epoll::del_handler(int fd)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        throw EpollExcepton(std::string("epoll_ctl error: ") + std::strerror(errno));
    }
}


} // namespace io

