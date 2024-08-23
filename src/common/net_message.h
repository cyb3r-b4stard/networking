#pragma once

#include "net_common.h"
#include "net_connection.h"

namespace net {

template <typename T>
class Connection;

/**
 * @brief Message Header, appears as the first part of any message
 * @param id - type of message
 * @param size - size of message in bytes
 * 
 * @tparam T - set of possible ids represented as an enum class
 */
template <typename T>
struct MessageHeader {
    T id;
    size_t size {0};
};

/**
 * @brief Message to send, contains information about message type and its contents
 * @param header - header of the message, determines type of the message and its size it bytes
 * @param contents - vector of bytes, contents of the message
 * 
 * @tparam T - set of possible ids represented as an enum class
 */
template <typename T>
struct Message {
    MessageHeader<T> header;
    std::vector<uint8_t> contents;

    inline size_t size() const {
        return sizeof(header) + contents.size();
    }

    /**
     * @brief Provide readable information about message for debug purposes
     * 
     * @param os - output stream
     * @param msg - message to deserialize
     * @return std::ostream& 
     */
    friend std::ostream& operator<<(std::ostream& os, const Message<T>& msg) {
        os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size;
        return os;
    }

    /**
     * @brief Update message with contents from provided data
     * 
     * @tparam DataType - type of data to add
     * @param msg - message to be updated
     * @param data - data to add
     * @return Message<T>& 
     */
    template <typename DataType>
    friend Message<T>& operator<<(Message<T>& msg, const DataType& data) {
        // Check that provided data is trivially copyable
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to serialize");

        // Cache current size of the vector
        size_t currentSize {msg.contents.size()};

        // Resize vector to add new data
        msg.contents.resize(currentSize + sizeof(data));

        // Physically copy the data into newly allocated space
        std::memcpy(msg.contents.data() + currentSize, &data, sizeof(data));

        // Recalculate the message size
        msg.header.size = msg.size();

        return msg;
    }

    /**
     * @brief Extract information from message into provided user variable
     * 
     * @tparam DataType - type of data to extract
     * @param msg - message where get the data from
     * @param data - variable to write data into
     * @return Message<T>& 
     */
    template <typename DataType>
    friend Message<T>& operator>>(Message<T>& msg, DataType& data) {
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to deserialize");

        // Cache the location where the data block we want to extract starts
        size_t currentSize {msg.contents.size() - sizeof(data)};

        // Physically copy the data from the vector into the user variable
        std::memcpy(&data, msg.contents.data() + currentSize, sizeof(data));

        // Resize vector to free up space
        msg.contents.resize(currentSize);

        msg.header.size = msg.size();

        return msg;
    }
};

/**
 * @brief Message with pointer to Connection that sent it
 * 
 * @tparam T - set of possible ids represented as an enum class
 */
template <typename T>
struct OwnedMessage {
    std::shared_ptr<Connection<T>> remote {nullptr};
    Message<T> msg;

    explicit OwnedMessage(const std::shared_ptr<Connection<T>>& remote, const Message<T>& msg) 
        : remote(std::move(remote)), msg(msg) 
    {}

    /**
     * @brief Provide readable information about message for debug purposes
     * 
     * @param os - output stream
     * @param msg - message to deserialize
     * @return std::ostream& 
     */
    friend std::ostream& operator<<(std::ostream& os, const OwnedMessage<T>& msg) {
        os << msg.msg;
        return os;
    }
};

}