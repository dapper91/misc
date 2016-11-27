#ifndef __CLIENT_H
#define __CLIENT_H


#include <stdexcept>
#include <string>
#include <memory>
#include <map>
#include <socket.h>


namespace chat {


/*
 * Represents Client exception.
 */
class ClientException: public std::runtime_error {
public:
    ClientException(const std::string& what_arg):
        std::runtime_error(what_arg)
    { }
};


/*
 * Represents a chat client, contains its status, socket, nick name.
 * Non-copyable.
 * Not thread-safe.
 */
class Client {
public:
    enum class Status {
        ONLINE,
        OFFLINE
    };

    const size_t msg_max_size = 65535;                      // maximum message size to be accepted
    static const std::map<Status, std::string> status_str;  // status string representation

    Client(std::unique_ptr<net::Socket> sock_ptr):
        sock_ptr(std::move(sock_ptr))
    { }

    /*
     * Calls disconnets.
     */
   ~Client();

    Client(const Client&) = delete;

    Client& operator=(const Client&) = delete;

    /*
     * Sets user status online. Gets user nick name.
     */
    void connect();

    /*
     * Sets user status offline. Closes the socket.
     */
    void disconnect();

    /*
     * Sends message msg to the user.
     * params:
     *      msg - message to be sent
     */
    void send_message(const std::string& msg);

    /*
     * Receives message msg from the user.
     * returns received message
     */
    std::string recv_message();

    Status get_status() const;

    void set_status(Status s);

    std::string get_nick() const;

private:
    // protocol message header
    struct msg_header {
        uint16_t size;      // size of the following message
    } __attribute__ ((__packed__));

    Status status = Status::OFFLINE;
    std::shared_ptr<net::Socket> sock_ptr;
    std::string nick;
};


} // chat namespace


#endif // __CLIENT_H