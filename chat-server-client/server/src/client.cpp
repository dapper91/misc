#include <client.h>

#include <stdexcept>
#include <string>
#include <memory>
#include <map>
#include <boost/format.hpp>
#include <socket.h>
#include <logger.h>


using namespace logging;


namespace chat {


Client::~Client()
{
    disconnect();
}

void Client::connect()
{
    nick = recv_message();
    status = Status::ONLINE;
    Logger::get_instance()->info(str(boost::format("user %1% connected") % nick));
}

void Client::disconnect()
{
    status = Status::OFFLINE;
    sock_ptr->close();
    Logger::get_instance()->info(str(boost::format("user %1% disconnected") % nick));
}

void Client::send_message(const std::string& msg)
{
    if (msg.size() > msg_max_size) {
        throw ClientException("client send message error: message too long");
    }

    msg_header hdr;
    hdr.size = htons(msg.size());

    std::vector<char> hdr_buf((const char*)&hdr,
                              (const char*)&hdr + sizeof(msg_header));
    std::vector<char> msg_buf(msg.cbegin(), msg.cend());

    sock_ptr->sendall(hdr_buf);
    sock_ptr->sendall(msg_buf);
}

std::string Client::recv_message()
{
    msg_header hdr;
    std::vector<char> msg_buf;
    std::vector<char> hdr_buf;

    sock_ptr->recvall(hdr_buf, sizeof(msg_header));
    std::copy(hdr_buf.cbegin(), hdr_buf.cend(), (char*)&hdr);

    size_t size = ntohs(hdr.size);
    sock_ptr->recvall(msg_buf, size);

    return std::string(msg_buf.cbegin(), msg_buf.cend());
}

Client::Status Client::get_status() const
{
    return status;
}

void Client::set_status(Status s)
{
    status = s;
}

std::string Client::get_nick() const
{
    return nick;
}

const std::map<Client::Status, std::string> Client::status_str = {
    {Status::ONLINE,  "online"},
    {Status::OFFLINE, "offline"}
};


} // chat namespace