#include <socket.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


namespace net {


Socket::Socket()
{
    sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw SocketException(std::string("socket error: ") + std::strerror(errno));
    }
}

Socket::Socket(Socket&& other)
{
    addr = other.addr;
    std::swap(sockfd, other.sockfd);
}

void Socket::bind(const std::string& ip, uint16_t port)
{
    fill_sockaddr(&addr, ip, port);

    if (::bind(sockfd, &addr, sizeof(sockaddr)) != 0) {
        throw SocketException(std::string("socket bind error: ") + std::strerror(errno));
    }
}

void Socket::connect(const std::string& ip, uint16_t port)
{
    sockaddr remote;
    fill_sockaddr(&remote, ip, port);

    if (::connect(sockfd, &remote, sizeof(sockaddr)) != 0) {
        throw SocketException(std::string("socket connect error: ") + std::strerror(errno));
    }
}

void Socket::listen(int backlog)
{
    if (::listen(sockfd, backlog) != 0) {
        throw SocketException(std::string("socket listen error: ") + std::strerror(errno));
    }
}

std::unique_ptr<Socket> Socket::accept()
{
    sockaddr addr;
    socklen_t len = sizeof(sockaddr);

    int fd = ::accept(sockfd, &addr, &len);
    if (fd < 0 || len != sizeof(sockaddr)) {
        throw SocketException(std::string("socket accept error: ") + std::strerror(errno));
    }

    // Socket is not thread-safe, so using unique_ptr is safe
    return std::unique_ptr<Socket>(new Socket(fd, addr));
}

void Socket::Socket::set_nonblocking()
{
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        throw SocketException(std::string("socket fcntl error: ") + std::strerror(errno));
    }
}

void Socket::set_reuse()
{
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        throw SocketException(std::string("socket setsockopt error: ") + std::strerror(errno));
    }
}

void Socket::close()
{
    if (sockfd > 0) {
        if (::close(sockfd) != 0) {
            throw SocketException(std::string("socket close error: ") + std::strerror(errno));
        }
        sockfd = -1;
    }
}

ssize_t Socket::sendall(const std::vector<char>& buf)
{
    ssize_t sent = 0;
    while (sent != buf.size()) {
        sent += send(buf, sent);
    }

    return sent;
}

ssize_t Socket::send(const std::vector<char>& buf, size_t offset)
{
    ssize_t res = ::send(sockfd, buf.data() + offset, buf.size() - offset, MSG_NOSIGNAL);
    if (res == 0) {
        throw SocketException("socket send error: socket has been closed");
    }
    if (res == -1) {
        // if nonblock flag is set (data is not available now, try later)
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }
        if (errno == EPIPE) {
            throw SocketException("socket send error: socket has been unexpectedly closed");
        }
        throw SocketException(std::string("socket send error: ") + std::strerror(errno));
    }

    return res;
}

ssize_t Socket::recvall(std::vector<char>& buf, size_t size)
{
    ssize_t recved = 0;
    while(recved != size) {
        recved += recv(buf, size);
    }

    return recved;
}

ssize_t Socket::recv(std::vector<char>& buf, size_t max)
{
    ssize_t res;
    char tmp[buf_size];

    res = ::recv(sockfd, tmp, std::min(buf_size, max), MSG_NOSIGNAL);
    if (res == 0) {
        throw SocketException("socket recv error: socket has been closed");
    }
    if (res == -1) {
        // if nonblock flag is set (data is not available now, try later)
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }
        if (errno == EPIPE) {
            throw SocketException("socket recv error: socket has been unexpectedly closed");
        }
        throw SocketException(std::string("socket recv error: ") + std::strerror(errno));
    }
    std::copy(tmp, tmp + res, std::back_inserter(buf));

    return buf.size();
}

int Socket::get_sockfd() const
{
    return sockfd;
}

void Socket::fill_sockaddr(sockaddr* addr, const std::string& ip, uint16_t port)
{
    sockaddr_in* addr_in = (sockaddr_in*)addr;

    addr_in->sin_family = AF_INET;
    addr_in->sin_port = htons(port);

    int res = inet_pton(AF_INET, ip.c_str(), &addr_in->sin_addr);
    if (res == -1) {
        throw SocketException(std::string("socket inet_pton error: ") + std::strerror(errno));
    }
    else if (res == 0) {
        throw SocketException(std::string("socket inet_pton error: incorrect ip address"));
    }
}


} // namespace net