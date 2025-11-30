#pragma once
#include "HeapStats.h"
#include "PriorityQueue.h"
#include <cstddef>
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
    const HeapStructureStats& structure_stats() const { return stats_; }

private:
    std::vector<BinaryHeapNode*> heap_; // binary heap storing pointers to nodes
    HeapStructureStats stats_{};

    static int parent(int i) { return (i - 1) / 2; }
    static int left(int i) { return 2 * i + 1; }
    static int right(int i) { return 2 * i + 2; }
    static std::size_t compute_height(std::size_t nodes);

    void heapify_up(int i);
    void heapify_down(int i);
    void swap_at(int i, int j);
    void update_size_metrics();
};
