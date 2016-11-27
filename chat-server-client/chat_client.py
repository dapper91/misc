#!/usr/bin/python2.7

import struct
import argparse
import sys
import select
import socket


# ChatClient base exception
class ChatClientException(Exception):
    pass


# Chat client
class ChatClient(object):

    MAX_MSG_LEN = 65535 # max message length can be received or sent

    # Chat client constructor.
    # params:
    #   host    - chat server ip to connect to
    #   port    - chat server port to connect to  
    #   nick    - user nick name
    #   timeout - socket operation timeout
    def __init__(self, host, port, nick, timeout = 2):
        self.stop_flag = False

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        self.socket.settimeout(timeout)
        self.socket.connect((host, port))
        
        # send user nick name first
        self.send_msg(nick)

        self.event_handlers = {
            sys.stdin.fileno():      self.on_stdin_event,
            self.socket.fileno():    self.on_socket_event
        }

        self.cmd_handlers = {
            'bye':  self.on_exit
        }

    # Sends message (header + message string) to chat server.
    # params:
    #   msg - message to send
    def send_msg(self, msg):
        self.socket.sendall(struct.pack('!H', len(msg)))
        self.socket.sendall(msg)

    # Receives message (header + message string) from chat server.
    # returns message string
    def recv_msg(self):
        data = self.recvall(2)        
        size = struct.unpack('!H', data)[0]
        return self.recvall(size)

    # Receives an entire message with size size from a socket.
    # params:
    #   size - message size to receive
    def recvall(self, size):
        data = ''
        while len(data) != size:
            data += self.socket.recv(size - len(data))
            if not data:
                raise ChatClientException("socket has been closed")
        return data

    # Register event handlers for stdin and server socket. 
    # Starts epoll event loop and dispatches events.
    def start(self):
        selector = select.epoll()
        selector.register(sys.stdin,   select.EPOLLIN)
        selector.register(self.socket, select.EPOLLIN)

        while not self.stop_flag:
            events = selector.poll()
            for fd, event in events:
                self.event_handlers[fd](event)

        self.socket.close()


    # Reads message from console and send it to server socket.
    # Invokes message hanlders for commands like bye.
    def on_stdin_event(self, event):
        if event & select.EPOLLERR:
            raise ChatClientException("stdin read error")

        msg = sys.stdin.readline().rstrip('\n')
        if len(msg) == 0:
            return
        if len(msg) > self.MAX_MSG_LEN:
            raise ChatClientException("message too long")
        if msg in self.cmd_handlers: 
            msg = self.cmd_handlers[msg](msg)        

        self.send_msg(msg)

    # Reads message from a socket and print it to console.
    # Handles errors.
    def on_socket_event(self, event):
        if event & select.EPOLLERR:
            raise ChatClientException("socket read error")
        elif event & select.EPOLLHUP:
            raise ChatClientException("socket has been closed")
        elif event & select.EPOLLIN:
            print(self.recv_msg())
        else:
            raise ChatClientException("got unknown event")
    
    # Bye command handler.
    def on_exit(self, msg):
        self.stop_flag = True
        return msg




def main():
    parser = argparse.ArgumentParser(description = 'Simple chat client.')
    parser.add_argument('-s', '--server', required = True, help = 'chat server host')
    parser.add_argument('-p', '--port', required = True, type = int, help = 'chat server port')
    parser.add_argument('-n', '--nick', required = True, help = 'user nick name')
    args = parser.parse_args()

    try:
        client = ChatClient(args.server, args.port, args.nick)
        print("successfuly connected to %s:%s" % (args.server, args.port))
        client.start()
        print("exiting")
    except socket.error as e:
        print("socket error: %s" % e)
    except socket.timeout:
        print("socket operation timeout")
    except ChatClientException as e:
        print(e)
    except KeyboardInterrupt as e:
        print("interrupted")


if __name__ == '__main__':
    main()