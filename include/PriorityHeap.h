#ifndef PRIORITY_HEAP_H
#define PRIORITY_HEAP_H

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

using namespace std;

// An order with a priority value.
// Higher priority means it should be processed earlier.
class PriorityItem {
public:
    int orderId;
    int priority;

    PriorityItem(int id = 0, int p = 0) {
        orderId = id;
        priority = p;
    }
};

// A stock and its traded volume, used for Top-K volume tracking.
class StockVolume {
public:
    string ticker;
    long long volume;

    StockVolume(string name = "", long long v = 0) {
        ticker = name;
        volume = v;
    }
};

// Comparator: larger priority should appear at the root (Max Heap).
class HigherPriorityFirst {
public:
    bool operator()(const PriorityItem& a, const PriorityItem& b) const {
        return a.priority > b.priority;
    }
};

// Comparator: smaller priority should appear at the root (Min Heap).
class LowerPriorityFirst {
public:
    bool operator()(const PriorityItem& a, const PriorityItem& b) const {
        return a.priority < b.priority;
    }
};

// Comparator: smaller volume should appear at the root.
// This is useful for keeping only the Top-K largest volumes.
class SmallerVolumeFirst {
public:
    bool operator()(const StockVolume& a, const StockVolume& b) const {
        return a.volume < b.volume;
    }
};

// Custom binary heap. Compare decides which element has higher heap priority.
template <typename T, typename Compare>
class PriorityHeap {
private:
    vector<T> heap;
    Compare hasHigherPriority;

    void siftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;

            if (!hasHigherPriority(heap[index], heap[parent])) {
                break;
            }

            swap(heap[index], heap[parent]);
            index = parent;
        }
    }

    void siftDown(int index) {
        int n = static_cast<int>(heap.size());

        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int best = index;

            if (left < n && hasHigherPriority(heap[left], heap[best])) {
                best = left;
            }

            if (right < n && hasHigherPriority(heap[right], heap[best])) {
                best = right;
            }

            if (best == index) {
                break;
            }

            swap(heap[index], heap[best]);
            index = best;
        }
    }

public:
    void push(const T& value) {
        heap.push_back(value);
        siftUp(static_cast<int>(heap.size()) - 1);
    }

    const T& top() const {
        if (heap.empty()) {
            throw out_of_range("Heap is empty");
        }
        return heap[0];
    }

    T pop() {
        if (heap.empty()) {
            throw out_of_range("Heap is empty");
        }

        T answer = heap[0];

        if (heap.size() == 1) {
            heap.pop_back();
            return answer;
        }

        heap[0] = heap.back();
        heap.pop_back();
        siftDown(0);

        return answer;
    }

    bool empty() const {
        return heap.empty();
    }

    int size() const {
        return static_cast<int>(heap.size());
    }
};

// Convenient names for the two heap types.
using MaxPriorityHeap = PriorityHeap<PriorityItem, HigherPriorityFirst>;
using MinPriorityHeap = PriorityHeap<PriorityItem, LowerPriorityFirst>;

// Returns the K stocks with the largest traded volumes, in descending order.
// The min heap keeps the smallest volume among the current Top-K at its root.
inline vector<StockVolume> getTopKVolumes(
    const vector<StockVolume>& stocks, int k
) {
    vector<StockVolume> answer;

    if (k <= 0) {
        return answer;
    }

    PriorityHeap<StockVolume, SmallerVolumeFirst> minHeap;

    for (int i = 0; i < static_cast<int>(stocks.size()); i++) {
        minHeap.push(stocks[i]);

        if (minHeap.size() > k) {
            minHeap.pop();
        }
    }

    while (!minHeap.empty()) {
        answer.push_back(minHeap.pop());
    }

    reverse(answer.begin(), answer.end());
    return answer;
}

#endif
