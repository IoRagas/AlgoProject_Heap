#include "BinaryHeap.h"

BinaryHeap::BinaryHeap() {}

BinaryHeap::~BinaryHeap() {
    for (auto* p : heap_) delete p;
}

BinaryHeapNode* BinaryHeap::insert(long long key, int value) {
    auto* node = new BinaryHeapNode{key, value, static_cast<int>(heap_.size())};
    heap_.push_back(node);
    heapify_up(node->index);
    return node;
}

std::pair<long long, int> BinaryHeap::extract_min() {
    if (heap_.empty()) throw std::runtime_error("extract_min from empty heap");
    BinaryHeapNode* root = heap_.front();
    std::pair<long long, int> result(root->key, root->value);

    BinaryHeapNode* last = heap_.back();
    heap_.pop_back();
    if (!heap_.empty()) {
        heap_[0] = last;
        last->index = 0;
        heapify_down(0);
    }

    delete root;
    return result;
}

std::pair<long long, int> BinaryHeap::peek_min() const {
    if (heap_.empty()) throw std::runtime_error("peek_min from empty heap");
    BinaryHeapNode* root = heap_.front();
    return {root->key, root->value};
}

void BinaryHeap::decrease_key(BinaryHeapNode* node, long long new_key) {
    if (!node) throw std::invalid_argument("node is null");
    if (new_key > node->key) throw std::invalid_argument("new_key is greater than current key");
    node->key = new_key;
    heapify_up(node->index);
}

void BinaryHeap::merge(PriorityQueue<BinaryHeapNode>& other_base) {
    auto* other = dynamic_cast<BinaryHeap*>(&other_base);
    if (!other) throw std::invalid_argument("merge requires another BinaryHeap");
    if (other == this || other->heap_.empty()) return;

    heap_.reserve(heap_.size() + other->heap_.size());
    for (auto* node : other->heap_) {
        node->index = static_cast<int>(heap_.size());
        heap_.push_back(node);
    }
    other->heap_.clear();

    if (heap_.empty()) return;
    for (int i = static_cast<int>(heap_.size() / 2) - 1; i >= 0; --i) {
        heapify_down(i);
    }
}

bool BinaryHeap::is_empty() const {
    return heap_.empty();
}

void BinaryHeap::swap_at(int i, int j) {
    std::swap(heap_[i], heap_[j]);
    heap_[i]->index = i;
    heap_[j]->index = j;
}

void BinaryHeap::heapify_up(int i) {
    while (i > 0) {
        int p = parent(i);
        if (heap_[p]->key <= heap_[i]->key) break;
        swap_at(p, i);
        i = p;
    }
}

void BinaryHeap::heapify_down(int i) {
    int n = static_cast<int>(heap_.size());
    while (true) {
        int l = left(i), r = right(i);
        int smallest = i;
        if (l < n && heap_[l]->key < heap_[smallest]->key) smallest = l;
        if (r < n && heap_[r]->key < heap_[smallest]->key) smallest = r;
        if (smallest == i) break;
        swap_at(i, smallest);
        i = smallest;
    }
}
