#ifndef __SOCKET_H
#define __SOCKET_H


#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>


namespace net {


/*
 * Represents Socket exception.
 */
class SocketException: public std::runtime_error {
public:
    SocketException(const std::string& what_arg):
        std::runtime_error(what_arg)
    { }
};

/*
 * Represents an ipv4 tcp socket.
 * Non-copyable.
 * Not thread-safe.
 */
class Socket {
public:
    Socket();

    Socket(int fd, sockaddr& addr):
        sockfd(fd), addr(addr)
    { }

    Socket(Socket&& other);

    Socket(const Socket&) = delete;

    Socket& operator=(const Socket&) = delete;

    void bind(const std::string& ip, uint16_t port);

    void connect(const std::string& ip, uint16_t port);

    void listen(int backlog = 64);

    std::unique_ptr<Socket> accept();

    void set_nonblocking();

    void set_reuse();

    void close();

    /*
     * Sends all the data in the buffer buf. Blocks untill all the data are sent.
     * params:
     *      buf - buffer containing data to be sent
     */
    ssize_t sendall(const std::vector<char>& buf);

    /*
     * Sends data in the buffer buf.
     * params:
     *      buf    - buffer containing the data to be sent
     *      offset - buffer data start position to be sent from
     * return a data size actually sent
     */
    ssize_t send(const std::vector<char>& buf, size_t offset = 0);

    /*
     * Receives data of required size from the socket. Blocks untill all the data are received.
     * params:
     *      buf  - buffer to save the received data to
     *      size - required data size to be received
     * returns received data size
     */
    ssize_t recvall(std::vector<char>& buf, size_t size);

    /*
     * Receives data from the socket.
     * params:
     *      buf - buffer to save the received data to
     *      max - maximum data size to be received
     * returns actually received data size
     */
    ssize_t recv(std::vector<char>& buf, size_t max = -1);

    int get_sockfd() const;

private:
    int sockfd = -1;
    sockaddr addr;
    size_t buf_size = 512;

    void fill_sockaddr(sockaddr* addr, const std::string& ip, uint16_t port);
};


} // namespace net


#endif // __SOCKET_H