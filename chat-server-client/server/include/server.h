#ifndef __SERVER_H
#define __SERVER_H


#include <string>
#include <vector>

#include <socket.h>
#include <epoll.h>
#include <queue.hpp>
#include <logger.h>
#include <client.h>


namespace chat {


/*
 * Represents a ChatServer exception.
 */
class ChatServerException: public std::runtime_error {
public:
    ChatServerException(const std::string& what_arg):
        std::runtime_error(what_arg)
    { }
};


/*
 *   //                        Chat server architecture:                       //
 *   //========================================================================//
 *   //   _______________                             ______________________   //
 *   //  |               | --------in_queue--------> |                      |  //
 *   //  |  io_handler   | (in-messages to process)  |   message_handler    |  //
 *   //  | (sockets I/O) | <-------out_queue-------- | (processes commands) |  //
 *   //  |_______________|  (out-messages to send)   |______________________|  //
 *   //                                                                        //
 *   //========================================================================//
 *
 *
 * Represents a chat server. Starts two threads: io_handler and message_handler.
 * The first one uses Epoll to dispatch I/O events to an approptiate handler;
 * receives data from client sockets, creates messages (see ChatServer::Message)
 * and sends it to in_queue; receives messages from out_queue and sends it to
 * message destination user sockets.
 * The second one processes commands received from in_queue, creates messages
 * and sends them to out_queue.
 * params:
 *      iface               - interface the server will be listenig on
 *      port                - port the server will be listenig on
 *      max_clients         - epoll max file descriprots
 *      listen_queue_size   - server socket listen queue size
 */
class ChatServer {
public:
    ChatServer(const std::string& iface, uint16_t port,
        size_t max_clients = 128, size_t listen_queue_size = 64);

    void start();

    void stop();

private:

    /*
     * Represents a message to be passed between io_handler and message_handler
     * through in_queue or out_queue
     */
    class Message {
    public:
        Message(const std::string& msg, const std::string& src):
            msg(msg), src(src)
        { }

        std::string get_message() const
        {
            return msg;
        }

        std::string get_source() const
        {
            return src;
        }

        std::vector<std::string> get_destinations() const
        {
            return dsts;
        }

        void add_destination(const std::string& dst)
        {
            dsts.push_back(dst);
        }

    private:
        std::string msg;                 // message text
        std::string src;                 // message source client name
        std::vector<std::string> dsts;   // mesasge destination clients name
    };

    typedef std::shared_ptr<Message> MessagePtr;
    typedef std::unique_ptr<Client> ClientPtr;

    io::Epoll epoll;

    net::Socket server_sock;
    std::unordered_map<std::string, ClientPtr> clients;

    concurrent::Queue<MessagePtr> in_queue;
    concurrent::Queue<MessagePtr> out_queue;

    bool stop_flag;

    /*
     * see above
     */
    void message_handler();

    /*
     * process list command
     */
    void on_list(MessagePtr msg_ptr);

    /*
     * creates broadcast message (a message to be sent to all online users)
     */
    void on_send(MessagePtr msg_ptr);

    std::string get_status_list();

    /*
     * see above
     */
    void io_handler();

    /*
     * handler to be called by io_handler on out_queue message pushed by message_handler
     */
    void on_queue_available(int events, void* data);

    /*
     * handler to be called by io_handler on client socket connetion
     */
    void on_client_connect(int events, void* data);

    /*
     * handler to be called by io_handler on client socket data received
     */
    void on_socket_data_available(int events, void* data);
};


} // namespace chat


#endif // __SERVER_H