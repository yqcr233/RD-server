#ifndef QUEUE_HPP
#define QUEUE_HPP
#include "core.hpp"

template<typename T>
class DataQueue{
    public:
        explicit DataQueue(size_t max_size_ = 1);
        DataQueue(const DataQueue<T>&) = delete;
        DataQueue(DataQueue<T>&&) = delete;
        DataQueue<T>& operator=(const DataQueue<T>&) = delete;
        DataQueue<T>& operator=(DataQueue<T>&&) = delete;
        ~DataQueue() = default;

        bool push(const T &obj);
        bool pop(T &obj);
        bool empty();
        void close();
        bool isClose();

    private:
        bool stop = false; 
        const size_t max_size; // 模板队列最大容量
        queue<T> queue;
        condition_variable is_full, is_empty;
        std::mutex mutex;
};

template class DataQueue<std::shared_ptr<AVPacket>>;  // 之前已有的
template class DataQueue<Microsoft::WRL::ComPtr<ID3D11Texture2D>>; // 新增

#endif






