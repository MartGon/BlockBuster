#pragma once

#include <vector>
#include <optional>
#include <functional>
#include <cstdint>

namespace Util
{
    // TODO: Redo using and index, instead of using std::vector::erase
    template <typename T>
    class CircularVector
    {
    public:
        CircularVector() = default;
        CircularVector(uint32_t capacity) : capacity_{capacity}
        {
            vector_.reserve(capacity);
        }

        inline uint32_t GetSize() const
        {
            return vector_.size();
        }
        
        inline uint32_t Empty() const
        {
            return vector_.empty();
        }

        inline uint32_t GetCapacity() const
        {
            return capacity_;
        }

        void Set(int32_t index, T val)
        {
            index = index >= 0 ? index : GetSize() + index;
            if(index < vector_.size())
                vector_[index] = val;
        }

        std::optional<T> At(int32_t index) const
        {
            std::optional<T> ret;
            index = index >= 0 ? index : GetSize() + index;
            if(index < vector_.size())
                ret = vector_[index];

            return ret;
        }

        inline std::optional<T> Front() const
        {
            return At(0);
        }

        inline std::optional<T> Back() const
        {
            return At(-1);
        }

        void PushBack(T val)
        {
            if(vector_.size() >= capacity_)
                vector_.erase(vector_.begin());

            vector_.push_back(val);
        }

        std::optional<T> PopFront()
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

        std::vector<T> Find(std::function<bool(T)> pred)
        {
            std::vector<T> ret;
            for(auto m : vector_)
            {
                if(pred(m))
                    ret.push_back(m);
            }

            return ret;
        }

        std::optional<T> FindFirst(std::function<bool(T)> pred)
        {
            std::optional<T> ret;

            for(auto m : vector_)
            {
                if(pred(m))
                {
                    ret = m;
                    break;
                }
            }

            return ret;
        }

        std::optional<T> FindRevFirst(std::function<bool(T)> pred)
        {
            std::optional<T> ret;

            for(auto it = vector_.rbegin(); it != vector_.rend(); it++)
            {
                if(pred(*it))
                {
                    ret = *it;
                    break;
                }
            }

            return ret;
        }

        std::vector<std::pair<uint32_t, T>> FindPairs(std::function<bool(uint32_t, T)> pred)
        {
            std::vector<std::pair<uint32_t, T>> ret;
            for(auto i = 0; i < vector_.size(); i++)
            {
                auto m = vector_[i];
                if(pred(i, m))
                    ret.push_back({i, m});
            }

            return ret;
        }

        std::optional<std::pair<uint32_t, T>> FindFirstPair(std::function<bool(uint32_t, T)> pred)
        {
            std::optional<std::pair<uint32_t, T>> ret;

            for(auto i = 0; i < vector_.size(); i++)
            {
                auto m = vector_[i];
                if(pred(i, m))
                {
                    ret = std::pair<uint32_t, T>{i, m};
                    break;
                }
            }

            return ret;
        }

        std::optional<std::pair<uint32_t, T>> FindRevFirstPair(std::function<bool(uint32_t, T)> pred)
        {
            std::optional<std::pair<uint32_t, T>> ret;

            if(!vector_.empty())
            {
                for(auto i = 0; i < vector_.size(); i++)
                {
                    auto index = (vector_.size() - 1) - i;
                    auto m = vector_[index];
                    if(pred(index, m))
                    {
                        ret = std::pair<uint32_t, T>{index, m};
                        break;
                    }
                }
            }

            return ret;
        }

        inline void Sort(std::function<bool(T, T)> pred)
        {
            std::sort(vector_.begin(), vector_.end(), pred);
        }

    private:
        std::vector<T> vector_;
        uint32_t capacity_ = 64;
    };
}