#pragma once

namespace Util
{

    template <typename T, int size>
    class Pool
    {
    public:

        T& Get()
        {
            return array[index];
        }

        int GetSize() const
        {
            return size;
        }
    
    private:

        int AdvanceIndex()
        {
            index = (index + 1) % size;
        }

        int index = 0;
        T array[size];
    };
}