#include "FibonacciHeap.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
FibonacciHeapNode* make_node(long long key, int value) {
    auto* node = new FibonacciHeapNode{};
    node->key = key;
    node->value = value;
    node->parent = nullptr;
    node->child = nullptr;
    node->left = node;
    node->right = node;
    node->degree = 0;
    node->mark = false;
    return node;
}

void concatenate_root_lists(FibonacciHeapNode* a, FibonacciHeapNode* b) {
    if (!a || !b) return;
    FibonacciHeapNode* aRight = a->right;
    FibonacciHeapNode* bLeft = b->left;

    a->right = b;
    b->left = a;
    aRight->left = bLeft;
    bLeft->right = aRight;
}
}

FibonacciHeap::FibonacciHeap() : min_(nullptr), size_(0) {}

FibonacciHeap::~FibonacciHeap() {
    delete_all(min_);
    min_ = nullptr;
    size_ = 0;
}

bool FibonacciHeap::is_empty() const {
    return min_ == nullptr;
}

FibonacciHeapNode* FibonacciHeap::insert(long long key, int value) {
    auto* node = make_node(key, value);
    add_to_root_list(node);
    ++size_;
    return node;
}

std::pair<long long, int> FibonacciHeap::extract_min() {
    if (!min_) {
        throw std::runtime_error("extract_min from empty FibonacciHeap");
    }

    FibonacciHeapNode* z = min_;
    if (z->child) {
        std::vector<FibonacciHeapNode*> children;
        FibonacciHeapNode* current = z->child;
        do {
            children.push_back(current);
            current = current->right;
        } while (current != z->child);

        for (auto* child : children) {
            child->parent = nullptr;
            child->mark = false;
            // detach child from its sibling ring
            child->left->right = child->right;
            child->right->left = child->left;
            child->left = child->right = child;
            add_to_root_list(child);
        }
        z->child = nullptr;
    }

    remove_from_root_list(z);
    --size_;

    if (min_) {
        consolidate();
    }

    std::pair<long long, int> result(z->key, z->value);
    delete z;
    return result;
}

std::pair<long long, int> FibonacciHeap::peek_min() const {
    if (!min_) {
        throw std::runtime_error("peek_min from empty FibonacciHeap");
    }
    return {min_->key, min_->value};
}

void FibonacciHeap::decrease_key(FibonacciHeapNode* node, long long new_key) {
    if (!node) {
        throw std::invalid_argument("node is null");
    }
    if (new_key > node->key) {
        throw std::invalid_argument("new_key is greater than current key");
    }

    node->key = new_key;
    FibonacciHeapNode* parent = node->parent;
    if (parent && node->key < parent->key) {
        cut(node, parent);
        cascading_cut(parent);
    }

    if (!min_ || node->key < min_->key) {
        min_ = node;
    }
}

void FibonacciHeap::merge(PriorityQueue<FibonacciHeapNode>& other_base) {
    auto* other = dynamic_cast<FibonacciHeap*>(&other_base);
    if (!other) throw std::invalid_argument("merge requires another FibonacciHeap");
    if (other == this || !other->min_) return;

    if (!min_) {
        min_ = other->min_;
        size_ = other->size_;
    } else {
        concatenate_root_lists(min_, other->min_);
        if (other->min_->key < min_->key) {
            min_ = other->min_;
        }
        size_ += other->size_;
    }

    other->min_ = nullptr;
    other->size_ = 0;
}

void FibonacciHeap::add_to_root_list(FibonacciHeapNode* node) {
    if (!node) return;
    if (!min_) {
        node->left = node->right = node;
        node->parent = nullptr;
        node->mark = false;
        min_ = node;
        return;
    }

    node->left = min_->left;
    node->right = min_;
    min_->left->right = node;
    min_->left = node;
    node->parent = nullptr;
    node->mark = false;
    if (node->key < min_->key) {
        min_ = node;
    }
}

void FibonacciHeap::remove_from_root_list(FibonacciHeapNode* node) {
    if (!node) return;
    if (node->right == node) {
        min_ = nullptr;
    } else {
        node->left->right = node->right;
        node->right->left = node->left;
        if (min_ == node) {
            min_ = node->right;
        }
    }
    node->left = node->right = node;
}

void FibonacciHeap::link_nodes(FibonacciHeapNode* child, FibonacciHeapNode* parent) {
    remove_from_root_list(child);
    child->parent = parent;
    child->mark = false;
    if (!parent->child) {
        parent->child = child;
        child->left = child->right = child;
    } else {
        child->left = parent->child->left;
        child->right = parent->child;
        parent->child->left->right = child;
        parent->child->left = child;
    }
    parent->degree++;
}

void FibonacciHeap::consolidate() {
    if (!min_) return;

    std::vector<FibonacciHeapNode*> roots;
    FibonacciHeapNode* current = min_;
    if (current) {
        do {
            roots.push_back(current);
            current = current->right;
        } while (current != min_);
    }

    std::size_t max_degree = 0;
    std::size_t n = size_;
    while (n > 0) {
        n >>= 1U;
        ++max_degree;
    }
    max_degree += 2;

    std::vector<FibonacciHeapNode*> degree_table(max_degree, nullptr);

    for (auto* w : roots) {
        FibonacciHeapNode* x = w;
        std::size_t d = static_cast<std::size_t>(x->degree);
        while (true) {
            if (d >= degree_table.size()) {
                degree_table.resize(d + 1, nullptr);
            }
            if (!degree_table[d]) break;
            FibonacciHeapNode* y = degree_table[d];
            if (x->key > y->key) std::swap(x, y);
            link_nodes(y, x);
            degree_table[d] = nullptr;
            ++d;
        }
        degree_table[d] = x;
    }

    min_ = nullptr;
    for (auto* node : degree_table) {
        if (!node) continue;
        node->left = node->right = node;
        node->parent = nullptr;
        node->mark = false;
        add_to_root_list(node);
    }
}

void FibonacciHeap::cut(FibonacciHeapNode* node, FibonacciHeapNode* parent) {
    if (!node || !parent) return;

    if (node->right == node) {
        parent->child = nullptr;
    } else {
        if (parent->child == node) {
            parent->child = node->right;
        }
        node->left->right = node->right;
        node->right->left = node->left;
    }
    parent->degree--;
    node->left = node->right = node;
    node->parent = nullptr;
    node->mark = false;
    add_to_root_list(node);
}

void FibonacciHeap::cascading_cut(FibonacciHeapNode* node) {
    FibonacciHeapNode* parent = node->parent;
    if (!parent) return;
    if (!node->mark) {
        node->mark = true;
    } else {
        cut(node, parent);
        cascading_cut(parent);
    }
}

void FibonacciHeap::delete_all(FibonacciHeapNode* node) {
    if (!node) return;
    FibonacciHeapNode* start = node;
    FibonacciHeapNode* current = start;
    do {
        FibonacciHeapNode* next = current->right;
        if (current->child) {
            delete_all(current->child);
            current->child = nullptr;
        }
        delete current;
        current = next;
    } while (current != start);
}

