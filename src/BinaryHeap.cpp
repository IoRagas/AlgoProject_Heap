#include "BinaryHeap.h"

BinaryHeap::BinaryHeap() {}

BinaryHeap::~BinaryHeap() {
    for (auto* p : heap_) delete p;
}

BinaryHeapNode* BinaryHeap::insert(long long key, int value) {
    auto* node = new BinaryHeapNode{key, value, static_cast<int>(heap_.size())};
    heap_.push_back(node);
    ++live_nodes_;
    heapify_up(node->index);
    update_size_metrics();
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
    if (live_nodes_ > 0) {
        --live_nodes_;
    }
    update_size_metrics();
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
    update_size_metrics();
}

bool BinaryHeap::is_empty() const {
    return heap_.empty();
}

void BinaryHeap::swap_at(int i, int j) {
    std::swap(heap_[i], heap_[j]);
    heap_[i]->index = i;
    heap_[j]->index = j;
    stats_.link_operations++;
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
    bool rearranged = false;
    int n = static_cast<int>(heap_.size());
    while (true) {
        int l = left(i), r = right(i);
        int smallest = i;
        if (l < n && heap_[l]->key < heap_[smallest]->key) smallest = l;
        if (r < n && heap_[r]->key < heap_[smallest]->key) smallest = r;
        if (smallest == i) break;
        swap_at(i, smallest);
        i = smallest;
        rearranged = true;
    }
    if (rearranged) {
        stats_.consolidation_passes++;
    }
}

std::size_t BinaryHeap::compute_height(std::size_t nodes) {
    if (nodes == 0) return 0;
    std::size_t height = 0;
    while (nodes > 0) {
        nodes >>= 1U;
        ++height;
    }
    return height;
}

void BinaryHeap::update_size_metrics() {
    stats_.current_nodes = heap_.size();
    live_nodes_ = stats_.current_nodes;
    if (stats_.current_nodes > stats_.max_nodes) {
        stats_.max_nodes = stats_.current_nodes;
    }
    const std::size_t height = compute_height(stats_.current_nodes);
    if (height > stats_.max_tree_height) {
        stats_.max_tree_height = height;
    }
    const std::size_t roots = stats_.current_nodes > 0 ? 1u : 0u;
    if (roots > stats_.max_roots) {
        stats_.max_roots = roots;
    }
    stats_.current_bytes = live_nodes_ * sizeof(BinaryHeapNode) + heap_.capacity() * sizeof(BinaryHeapNode*);
    if (stats_.current_bytes > stats_.max_bytes) {
        stats_.max_bytes = stats_.current_bytes;
    }
}
