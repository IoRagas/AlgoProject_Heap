#pragma once
#include "PriorityQueue.h"
#include <vector>
#include <utility>
#include <stdexcept>

// Node handle type used by BinaryHeap and exposed to clients for decrease_key
struct BinaryHeapNode {
    long long key;
    int value;
    int index; // position in heap array
};

class BinaryHeap : public PriorityQueue<BinaryHeapNode>
{
public:
    BinaryHeap();
    ~BinaryHeap() override;

    BinaryHeapNode* insert(long long key, int value) override;
    std::pair<long long, int> extract_min() override;
    std::pair<long long, int> peek_min() const override;
    void decrease_key(BinaryHeapNode* node, long long new_key) override;
    void merge(PriorityQueue& other) override;
    bool is_empty() const override;

private:
    std::vector<BinaryHeapNode*> heap_; // binary heap storing pointers to nodes

    static int parent(int i) { return (i - 1) / 2; }
    static int left(int i) { return 2 * i + 1; }
    static int right(int i) { return 2 * i + 2; }

    void heapify_up(int i);
    void heapify_down(int i);
    void swap_at(int i, int j);
};
