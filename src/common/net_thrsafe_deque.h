#pragma once

#include "net_common.h"
#include <queue>

namespace net {

/**
 * @brief Thread safe double ended queue
 * 
 * @tparam T - value type
 */
template <typename T>
class ThrSafeDeque {
public:
    ThrSafeDeque() = default;
    ThrSafeDeque(const ThrSafeDeque<T>&) = delete;
    ~ThrSafeDeque() { clear(); }

    const T& front() {
        std::scoped_lock lock(mux);
        return deque.front();
    }

    const T& back() {
        std::scoped_lock lock(mux);
        return deque.back();
    }

    void push_back(const T& item) {
        std::scoped_lock lock(mux);
        deque.emplace_back(std::move(item));
    }

    void push_front(const T& item) {
        std::scoped_lock lock(mux);
        deque.emplace_front(std::move(item));
    }

    T& pop_back() {
        std::scoped_lock lock(mux);
        T t = std::move(deque.back());
        deque.pop_back();

        return t;
    }

    T& pop_front() {
        std::scoped_lock lock(mux);
        T t = std::move(deque.front());
        deque.pop_front();

        return t;
    }

    size_t size() {
        std::scoped_lock lock(mux);
        return deque.size();
    }

    bool empty() {
        std::scoped_lock lock(mux);
        return deque.empty();
    } 

    void clear() {
        std::scoped_lock lock(mux);
        deque.clear();
    }
private:
    std::deque<T> deque;
    std::mutex mux;
};

}