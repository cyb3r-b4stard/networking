#pragma once

#include "net_common.h"
#include "net_thrsafe_deque.h"
#include "net_message.h"
#include "net_connection.h"

namespace net {

template <typename T>
class InterfaceServer {
public:
    InterfaceServer(uint16_t port)
        : acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) 
    {}

    virtual ~InterfaceServer() {
        Stop();
    }

    bool Start() {
        try {
            WaitForClientConnection();
            thrContext = std::thread([this](){context.run();});
        } catch (std::exception& e) {
            std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
            return false;
        }

        std::cout << "[SERVER] Started!" << std::endl;
        return true;
    }

    void Stop() {
        context.stop();

        if (thrContext.joinable()) {
            thrContext.join();
        }

        std::cout << "[SERVER] Stopped!" << std::endl;
    }

    void WaitForClientConnection() {
        acceptor.async_accept(
            [this](std::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;

                    std::shared_ptr<Connection<T>> newConnection = 
                        std::make_shared<Connection<T>>(Connection<T>::Owner::Server, context, std::move(socket), messagesIn);

                    if (OnClientConnect(newCOnnection)) {
                        connections.push_back(std::move(newConnection))

                        connections.back()->ConnectToClient(idCounter++);

                        std::cout << "|" << connections.back()->GetId() << "| Connection Approved" << std::endl;
                    } else {
                        std::cout << "[-----] Connection Denied" << std::endl;
                    }
                } else {
                    std::cout << "[SERVER] New Connection Error: " << ec.message() << std::endl;
                }

                WaitForClientConnection();
            }
        )
    }

    void MessageClient(std::shared_ptr<Connection<T>> client, const Message<T>& msg) {
        if (client && client->IsConnected()) {
            client->Send(msg);
        } else {
            OnClientDisconnect(client);
            client.reset();
            connections.erase(std::remove(connections.begin(), connections.end(), client), connections.end());
        }
    }

    void MessageAllClients(const Message<T>& msg, std::shared_ptr<Connection<T>> ignoreClient = nullptr) {
        bool invalidClientExists {false};

        for (auto& client : connections) {
            if (client && client->IsConnected()) {
                if (client != ignoreClient) {
                    client->Send(msg);
                }
            } else {
                OnClientDisconnect(client);
                client.reset();
                invalidClientExists = true;
            }
        }

        if (invalidClientExists) {
            connections.erase(std::remove(connections.begin(), connections.end(), nullptr), connections.end());
        }
    }

    void Update(size_t maxMessages = SIZE_MAX) {
        size_t messageCount {0};

        while (messageCount < maxMessages && !messagesIn.empty()) {
            auto msg = messagesIn.pop_front();

            OnMessage(msg.remote, msg.msg);

            ++messageCount;
        }
    }
protected:
    ThrSafeDeque<OwnedMessage<T>> messagesIn;

    std::deque<std::shared_ptr<Connection<T>>> connections;

    boost::asio::io_context context;
    std::thread thrContext;

    boost::asio::ip::tcp::acceptor acceptor;

    uint32_t idCounter {0};
    
    virtual bool OnClientConnect(std::shared_ptr<Connection<T>> client) {

    }

    virtual void OnClientDisconnect(std::shared_ptr<Connection<T>> client) {

    }

    virtual void OnMessage(std::shared_ptr<Connection<T>> client, Message<T>& msg) {

    }
};

}