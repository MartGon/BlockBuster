#pragma once

#include <vector>
#include <optional>
#include <cstdint>

namespace Util
{
    template <typename T>
    class Queue
    {
    public:
        Queue() = default;
        Queue(uint32_t capacity) : capacity_{capacity}
        {
            vector_.reserve(capacity);
        }

        inline uint32_t GetSize() const
        {
            return vector_.size();
        }

        inline uint32_t GetCapacity() const
        {
            return capacity_;
        }

        std::optional<T> At(int32_t index) const
        {
            std::optional<T> ret;
            index = index >= 0 ? index : GetSize() + index;
            if(index < vector_.size())
                ret = vector_[index];

            return ret;
        }

        std::optional<T> Front() const
        {
            return At(0);
        }

        std::optional<T> Back() const
        {
            return At(-1);
        }

        void Push(T val)
        {
            if(vector_.size() >= capacity_)
                vector_.erase(vector_.begin());

            vector_.push_back(val);
        }

        std::optional<T> Pop()
        {
            auto ret = Front();
            if(ret)
                vector_.erase(vector_.begin());

            return ret;
        }

        void Resize(uint32_t capacity)
        {
            auto prevSize = capacity_;
            capacity_ = capacity;

            auto diff = capacity_ - prevSize;
            for(auto i = 0; i < diff; i++)
                vector_.erase(vector_.begin());
        }

    private:
        std::vector<T> vector_;
        uint32_t capacity_ = 64;
    };
}