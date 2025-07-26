#include "data_queue.hpp"

using namespace std;

template<typename T>
DataQueue<T>::DataQueue(size_t max_size_) : max_size(max_size_){}

template<typename T> 
bool DataQueue<T>::push(const T &obj){
    unique_lock<std::mutex> lock(mutex);
    is_full.wait(lock, [this]->bool{
        return stop || queue.size() < max_size;
    });

    if(stop) return false;

    queue.emplace(obj);
    // fprintf(stdout, "push %s size: %d mx: %d\n", typeid(T).name(), queue.size(), max_size);
    lock.unlock();
    is_empty.notify_one();
    return true;
}

template<typename T>
bool DataQueue<T>::pop(T& obj){
    unique_lock<std::mutex> lock(mutex);
    is_empty.wait(lock, [this]->bool{
        return stop || queue.size() > 0; // 队列已经stop也要讲队列中剩余的元素出队
    });

    if(queue.empty()) return false; 

    obj = move(queue.front());
    queue.pop();
    // fprintf(stdout, "pop %s size: %d\n",typeid(T).name() ,queue.size());
    lock.unlock();
    is_full.notify_one();
    return true;
}

template<typename T>
bool DataQueue<T>::empty(){
    lock_guard lock(mutex);
    return queue.empty();
}

template<typename T>
bool DataQueue<T>::isClose(){
    lock_guard lock(mutex);
    return stop;
}

template<typename T>
void DataQueue<T>::close(){
    {
        lock_guard lock(mutex);
        stop = true;
    }
    is_full.notify_all();
    is_empty.notify_all();
}