#pragma once

#include "net_common.h"
#include "net_thrsafe_deque.h"
#include "net_message.h"

namespace net {

template <typename T>
struct Message;

template <typename T>
struct OwnedMessage;

template <typename T>
class Connection : public std::enable_shared_from_this<Connection<T>> {
public:
    enum class Owner {
        Server, Client
    };

    Connection(Owner owner, boost::asio::io_context& context, 
                boost::asio::ip::tcp::socket socket, ThrSafeDeque<OwnedMessage<T>>& messagesIn) 
        : context(context), socket(std::move(socket)), messagesIn(messagesIn), owner(owner)
    {}

    virtual ~Connection() {}


    void ConnectToClient(uint32_t id = 0) {
        if (owner == Owner::Server) {
            if (socket.is_open()) {
                id = id;
                ReadHeader();
            }
        }
    }

    void ConnectToServer(const boost::asio::ip::tcp::resolver::results_type& endpoints) {
        if (owner == Owner::Client) {
            boost::asio::async_connect(socket, endpoints, 
                [this](std::error_code ec, boost::asio::ip::tcp::endpoint endpoint) {
                    if (!ec) {
                        ReadHeader();
                    } else {
                        std::cout << "[" << id << "] Connection Fail" << std::endl;
                    }
                });
        }
    }

    void Disconnect() {
        boost::asio::post(context, [this]() {socket.close();});
    }
    
    bool IsConnected() const {
        return socket.is_open();
    }

    void Send(const Message<T>& msg) {
        boost::asio::post(context, 
            [this, &msg]() {
                bool writingMessage = !messagesOut.empty();

                messagesOut.push_back(msg);

                if (!writingMessage) {
                    WriteHeader();
                }
            });
    }

    inline uint32_t GetId() {
        return id;
    }
private:
    Owner owner {Owner::Server};
    uint32_t id {0};

    // Each connection has a unique socket to a remote
    boost::asio::ip::tcp::socket socket;

    // Context is shared with the whole asio instance
    boost::asio::io_context& context;

    // Messages to be sent to the remote side of the connection
    ThrSafeDeque<Message<T>> messagesOut;

    // Messages that have been received from the remote side of this connection.
    // Note that the reference implies that the owner of this connection 
    // is expected to provide with a queue
    ThrSafeDeque<OwnedMessage<T>>& messagesIn;
    Message<T> msgTemporaryIn;

    void ReadHeader() {
        boost::asio::async_read(socket, boost::asio::buffer(&msgTemporaryIn.header, sizeof(msgTemporaryIn.header)),
            [this](std::error_code ec, size_t size) {
                if (!ec) {
                    if (msgTemporaryIn.header.size > 0) {
                        msgTemporaryIn.contents.resize(msgTemporaryIn.header.size);
                        ReadContents();
                    } else {
                        AddToIncomingMessageQueue();
                    }
                } else {
                    std::cout << "[" << id << "] Read Header Fail" << std::endl;
                    socket.close(); 
                }
            });
    }

    void ReadContents() {
        boost::asio::async_read(socket, boost::asio::buffer(msgTemporaryIn.contents.data(), msgTemporaryIn.contents.size()),
            [this](std::error_code ec, size_t size) {
                if (!ec) {
                    AddToIncomingMessageQueue();
                } else {
                    std::cout << "[" << id << "] Read Contents Fail" << std::endl;
                    socket.close(); 
                }
            });
    }

    void WriteHeader() {
        boost::asio::async_write(socket, boost::asio::buffer(&messagesOut.front().header, sizeof(messagesOut.front().header)),
            [this](std::error_code ec, size_t size) {
                if (!ec) {
                    if (messagesOut.front().contents.size() > 0) {
                        WriteContents();
                    } else {
                        messagesOut.pop_front();

                        if (!messagesOut.empty()) {
                            WriteHeader();
                        }
                    }
                } else {
                    std::cout << "[" << id << "] Write Header Fail" << std::endl;
                    socket.close();
                }
            });
    }

    void WriteContents() {
        boost::asio::async_write(socket, boost::asio::buffer(&messagesOut.front().contents.data(), messagesOut.front().contents.size()),
            [this](std::error_code ec, size_t size) {
                if (!ec) {
                    messagesOut.pop_front();

                    if (!messagesOut.empty()) {
                        WriteHeader();
                    }
                } else {
                    std::cout << "[" << id << "] Write Contents Fail" << std::endl;
                    socket.close();
                }
            });
    }

    void AddToIncomingMessageQueue() {
        if (owner == Owner::Server) {
            messagesIn.push_back(OwnedMessage<T>(this->shared_from_this(), msgTemporaryIn));
        } else {
            messagesIn.push_back(OwnedMessage<T>(nullptr, msgTemporaryIn));
        }

        ReadHeader();
    }
};


}