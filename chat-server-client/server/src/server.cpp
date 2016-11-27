#include <server.h>

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <algorithm>
#include <boost/format.hpp>

#include <socket.h>
#include <epoll.h>
#include <queue.hpp>
#include <logger.h>
#include <client.h>


namespace chat {


using namespace std::placeholders;
using namespace logging;



ChatServer::ChatServer(const std::string& iface, uint16_t port,
    size_t max_clients, size_t listen_queue_size):
    epoll(max_clients)
{
    server_sock.bind(iface, port);
    server_sock.listen(listen_queue_size);
}

void ChatServer::start()
{
    std::thread io_thread(&ChatServer::io_handler, this);   // start io_handler in a new thread
    message_handler();                                      // start message handler in the current thread

    io_thread.join();
}

void ChatServer::stop()
{
    stop_flag = true;
}


void ChatServer::message_handler()
{
    auto msg_ptr = std::shared_ptr<Message>();

    while (!stop_flag) {
        in_queue.wait_pop(msg_ptr);

        Logger::get_instance()->debug("got message from user " + msg_ptr->get_source());

        if (msg_ptr->get_message() == "list") {
            on_list(msg_ptr);
        }
        else {
            on_send(msg_ptr);
        }
    }
}

void ChatServer::on_list(MessagePtr msg_ptr)
{
    // get a status of the users, format status list and send it back
    auto resp_msg_ptr = std::make_shared<Message>(get_status_list(), msg_ptr->get_source());
    resp_msg_ptr->add_destination(msg_ptr->get_source());
    out_queue.push(resp_msg_ptr);
}

void ChatServer::on_send(MessagePtr msg_ptr)
{
    auto resp_msg_ptr = std::make_shared<Message>(str(boost::format("%1%: %2%")
                                                        % msg_ptr->get_source()
                                                        % msg_ptr->get_message()), msg_ptr->get_source());

    // send the message to all online users but source
    for (const auto& nick_client_pair: clients) {
        if (nick_client_pair.first != msg_ptr->get_source() &&
            nick_client_pair.second->get_status() == Client::Status::ONLINE) {
            resp_msg_ptr->add_destination(nick_client_pair.first);
        }
    }
    out_queue.push(resp_msg_ptr);
}

std::string ChatServer::get_status_list()
{
    std::stringstream out;

    for (const auto& nick_client_pair: clients) {
        out << std::left
            << std::setw(10)
            << nick_client_pair.first
            << ": "
            << Client::status_str.at(nick_client_pair.second->get_status())
            << "\n";
    }

    return out.str();
}



void ChatServer::io_handler()
{
    auto handler1 = std::bind(&ChatServer::on_client_connect, this, _1, _2);
    epoll.add_handler(server_sock.get_sockfd(), io::Epoll::Event::IN, handler1);

    auto handler2 = std::bind(&ChatServer::on_queue_available, this, _1, _2);
    epoll.add_handler(out_queue.get_eventfd(),  io::Epoll::Event::IN, handler2);

    epoll.start();
}

void ChatServer::on_queue_available(int events, void* data)
{
    if ((events & io::Epoll::Event::ERR) ||
        (events & io::Epoll::Event::HUP)) {
        throw ChatServerException("epoll error: queue eventfd unexpected error occured");
    }

    auto msg_ptr = std::shared_ptr<Message>();

    if (out_queue.try_pop(msg_ptr)) {
        std::vector<std::string> dsts = msg_ptr->get_destinations();

        for (const std::string& dst: dsts) {
            try {
                clients[dst]->send_message(msg_ptr->get_message());
            }
            catch (ClientException& e) {
                Logger::get_instance()->warning(e.what());
                clients[dst]->disconnect();
            }
            catch (net::SocketException& e) {
                Logger::get_instance()->warning(e.what());
                clients[dst]->disconnect();
            }
        }
    }
}

void ChatServer::on_client_connect(int events, void* data)
{
    if ((events & io::Epoll::Event::ERR) ||
        (events & io::Epoll::Event::HUP)) {
        throw ChatServerException("epoll error: server socket unexpected error occured");
    }

    auto client_sock_ptr = server_sock.accept();
    int sock_fd = client_sock_ptr->get_sockfd();

    auto client_ptr = std::unique_ptr<Client>(new Client(std::move(client_sock_ptr)));

    try {
        client_ptr->connect();
        auto handler = std::bind(&ChatServer::on_socket_data_available, this, _1, _2);
        // handler will be called in the current thread before client_ptr is destructed,
        // therefore we don't get dangling pointer, so using client_ptr.get() is safe.
        epoll.add_handler(sock_fd, io::Epoll::Event::IN |
                                   io::Epoll::Event::RDHUP, handler, client_ptr.get());
        clients[client_ptr->get_nick()] = std::move(client_ptr);
    }
    catch (ClientException& e) {
        Logger::get_instance()->warning(e.what());
        client_ptr->disconnect();
    }
    catch (net::SocketException& e) {
        Logger::get_instance()->info(e.what());
        client_ptr->disconnect();
    }
}

void ChatServer::on_socket_data_available(int events, void* data)
{
    Client* client_ptr = static_cast<Client*>(data);

    if (events & io::Epoll::Event::ERR) {
        Logger::get_instance()->warning("epoll error: client socket unexpected error occured");
        client_ptr->disconnect();
    }
    else if ((events & io::Epoll::Event::HUP) || (events & io::Epoll::Event::RDHUP)) {
        Logger::get_instance()->debug("epoll: client socket has been closed by the remote peer");
        client_ptr->disconnect();
    }
    else {
        try {
            in_queue.push(std::make_shared<Message>(client_ptr->recv_message(), client_ptr->get_nick()));
        }
        catch (ClientException& e) {
            Logger::get_instance()->warning(e.what());
            client_ptr->disconnect();
        }
        catch (net::SocketException& e) {
            Logger::get_instance()->warning(e.what());
            client_ptr->disconnect();
        }
    }
}


} // namespace chat