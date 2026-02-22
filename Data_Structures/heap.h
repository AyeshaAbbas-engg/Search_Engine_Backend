#ifndef HEAP_H
#define HEAP_H

#include <vector>
#include <algorithm>

template <typename T, typename Compare>
class MaxHeap {
private:
    std::vector<T> data;
    Compare comp;

    void siftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (comp(data[parent], data[index])) {
                std::swap(data[parent], data[index]);
                index = parent;
            } else break;
        }
    }

    void siftDown(int index) {
        int size = data.size();
        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int largest = index;
            if (left < size && comp(data[largest], data[left])) largest = left;
            if (right < size && comp(data[largest], data[right])) largest = right;
            if (largest != index) {
                std::swap(data[index], data[largest]);
                index = largest;
            } else break;
        }
    }

public:
    void push(const T& value) {
        data.push_back(value);
        siftUp(data.size() - 1);
    }

    T pop() {
        if (data.empty()) return T();
        T topValue = data[0];
        if (data.size() > 1) {
            data[0] = data.back();
            data.pop_back();
            siftDown(0);
        } else {
            data.pop_back();
        }
        return topValue;
    }

    const T& top() const { return data[0]; }
    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }
};

#endif