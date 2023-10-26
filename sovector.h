#pragma once

#include <vector>

template <typename T, std::size_t stackSize = 16> class sovector
{
private:
    T stackData[stackSize];
    std::vector<T> heapData;

    std::size_t size;

public:
    std::size_t heapReserveSize = stackSize;

    sovector() : size(0) {}

    sovector(std::size_t initialSize) { reserve(initialSize); }

    void push_back(const T &value)
    {
        if (size < stackSize)
        {
            stackData[size] = value;
        }
        else
        {
            if (size == stackSize)
            {
                // Move existing stack data to heap
                /* heapData.assign(stackData, stackData + stackSize); */
                heapData.reserve(heapReserveSize);
            }
            heapData.push_back(value);
        }
        size++;
    }

    void reserve(std::size_t size)
    {
        if (size <= this->size)
        {
            return;
        }

        heapData.reserve(size - this->size);

        this->size = size;
    }

    T &operator[](std::size_t index)
    {
        if (index < stackSize)
        {
            return stackData[index];
        }
        else
        {
            return heapData[index - stackSize];
        }
    }

    std::size_t getSize() const { return size; }
};