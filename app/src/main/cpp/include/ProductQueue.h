//
// Created by pc on 2024/5/13.
//

#ifndef ANDROIDPLAYER_PRODUCTQUEUE_H
#define ANDROIDPLAYER_PRODUCTQUEUE_H


#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>

template<typename T>
class ProductQueue {
private:
    std::vector<T> products;
    int max_size;
    std::atomic<int> count;
    int head;
    int tail;
    std::mutex mtx; // 添加互斥量

public:
    ProductQueue(int capacity);
    ProductQueue(int capacity, T(* initFunction)());
    void push(const T item);
    T pop();
    void clear();
};

template<typename T>
void ProductQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(mtx); // 使用互斥量保护关键部分
    count = 0;
    head = 0;
    tail = 0;
}


template<typename T>
ProductQueue<T>::ProductQueue(int capacity) : max_size(capacity), count(0), head(0), tail(0), products(capacity) {}

template<typename T>
ProductQueue<T>::ProductQueue(int capacity, T (*initFunction)()) : max_size(capacity), count(0), head(0), tail(0), products(capacity) {
    for(int i = 0; i < capacity; i++) {
        products[i] = initFunction();
    }
}

template<typename T>
void ProductQueue<T>::push(const T item) {
    int currCount;
    {

        currCount = count.load(std::memory_order_relaxed);
        while (currCount >= max_size) { // Check if queue is full
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep for 10ms
            currCount = count.load(std::memory_order_relaxed);
        }
        std::lock_guard<std::mutex> lock(mtx); // 使用互斥量保护关键部分
        products[tail] = item;
        tail = (tail + 1) % max_size;
        count.store(currCount + 1, std::memory_order_relaxed);
    }
}

template<typename T>
T ProductQueue<T>::pop() {
    int currCount;
    T item;
    {

        currCount = count.load(std::memory_order_relaxed);
        while (currCount <= 0) { // Check if queue is empty
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep for 10ms
            currCount = count.load(std::memory_order_relaxed);
        }
        std::lock_guard<std::mutex> lock(mtx); // 使用互斥量保护关键部分
        item = products[head];
        head = (head + 1) % max_size;
        count.store(currCount - 1, std::memory_order_relaxed);
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return item;
}





//#include <queue>
//#include <mutex>
//#include <chrono>
//#include <thread>
//
//template<typename T>
//class ProductQueue {
//private:
//    std::queue<T> products;
//    int max_size;
//    std::mutex mtx; // Mutex for thread safety
//
//public:
//    ProductQueue(int capacity);
//    ProductQueue(int capacity, T(* initFunction)());
//    void push(const T& item);
//    T pop();
//};
//
//template<typename T>
//ProductQueue<T>::ProductQueue(int capacity) : max_size(capacity) {}
//
//template<typename T>
//ProductQueue<T>::ProductQueue(int capacity, T (*initFunction)()) : max_size(capacity) {
//    for(int i = 0; i < capacity; i++) {
//        products.push(initFunction());
//    }
//}
//
//template<typename T>
//void ProductQueue<T>::push(const T &item) {
//    std::unique_lock<std::mutex> lock(mtx); // Lock the mutex
//    if (products.size() >= max_size) {
//        lock.unlock(); // Unlock before sleeping
//        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep for 10ms
//        lock.lock(); // Re-lock after sleeping
//    }
//    products.push(item);
//}
//
//template<typename T>
//T ProductQueue<T>::pop() {
//    std::unique_lock<std::mutex> lock(mtx); // Lock the mutex
//    while (products.empty()) {
//        lock.unlock(); // Unlock before sleeping
//        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep for 10ms
//        lock.lock(); // Re-lock after sleeping
//    }
//    T item = products.front();
//    products.pop();
//    return item;
//}


#endif //ANDROIDPLAYER_PRODUCTQUEUE_H
