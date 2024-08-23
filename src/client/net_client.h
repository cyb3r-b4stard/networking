#pragma once

#include "net_common.h"
#include "net_thrsafe_deque.h"
#include "net_message.h"

namespace net {

template <typename T>
class InterfaceClient {
public:
    InterfaceClient() 
        : socket(context) 
    {}

    virtual ~InterfaceClient() {
        Disconnect();
    }

    bool Connect(const std::string& host, const uint16_t port) {
        try {
            // Create connection
            connection = std::make_unique<Connection<T>>();

            // Resolve hostname/ip-address into physical address
            boost::asio::ip::tcp::resolver(context);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            // Create connection
            connection = std::make_unique<Connection<T>>(
                Connection<T>::Owner::Client, context, boost::asio::ip::tcp::socket(context), messagesIn);

            // Tell the connection object to connect to server
            connection->ConnectToServer(endpoints);

            // Start context thread
            thrContext = std::thread([this](){context.run();});
        } catch (std::exception& e) {
            std::cerr << "Client Exception: " << e.what() << std::endl;
            return false
        }

        return true;
    }

    bool Disconnect() {
        if (IsConnected()) {
            connection->Disconnect();
        }

        // In either way asio context is no longer needed
        context.stop();

        if (thrContext.joinable()) {
            thrContext.join();
        }

        connection.release();
    }
    bool IsConnected() {
        return (connection) ? connection->IsConnected() : false;
    }

    ThrSafeDeque<OwnedMessage<T>>& Incoming() {
        return messagesIn;
    }
protected:
    // Asio context that handles data transfer
    boost::asio::io_context context;

    // Separate thread for the context to execute its work commands
    std::thread thrContext;

    // Hardware socket that is connected to the server
    boost::asio::ip::tcp::socket socket;

    // The client has a single connection that handles data transfer
    std::unique_ptr<Connection<T>> connection;
private:
    // Messages from the server
    ThrSafeDeque<OwnedMessage<T>> messagesIn;
};
    
}