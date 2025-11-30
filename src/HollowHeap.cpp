#include "HollowHeap.h"

#include <algorithm>
#include <stdexcept>

struct HollowHeapCell {
    long long key;
    int value;
    HollowHeapCell* child;
    HollowHeapCell* next;
    HollowHeapCell* second_parent;
    unsigned rank;
    bool hollow;
    HollowHeapNode* owner;
};

namespace {
constexpr std::size_t kInitialRankCapacity = 16;
}

HollowHeap::HollowHeap()
    : root_(nullptr), active_size_(0) {
    rankmap_.assign(kInitialRankCapacity, nullptr);
    to_delete_.reserve(32);
}

HollowHeap::~HollowHeap() = default;

bool HollowHeap::is_empty() const {
    return root_ == nullptr;
}

HollowHeapNode* HollowHeap::make_handle() {
    handles_.push_back(std::make_unique<HollowHeapNode>());
    return handles_.back().get();
}

HollowHeapCell* HollowHeap::make_cell(long long key, int value, HollowHeapNode* owner) {
    auto cell = std::make_unique<HollowHeapCell>();
    HollowHeapCell* raw = cell.get();
    raw->key = key;
    raw->value = value;
    raw->child = nullptr;
    raw->next = nullptr;
    raw->second_parent = nullptr;
    raw->rank = 0;
    raw->hollow = false;
    raw->owner = owner;
    cells_.push_back(std::move(cell));
    return raw;
}

void HollowHeap::ensure_rank_capacity(std::size_t rank) {
    if (rank >= rankmap_.size()) {
        rankmap_.resize(rank + 1, nullptr);
    }
}

void HollowHeap::clear_rankmap() {
    std::fill(rankmap_.begin(), rankmap_.end(), nullptr);
}

HollowHeapCell* HollowHeap::link(HollowHeapCell* u, HollowHeapCell* v) {
    if (!u) return v;
    if (!v) return u;

    HollowHeapCell* parent = u;
    HollowHeapCell* child = v;

    if (v->key < u->key) {
        parent = v;
        child = u;
    } else if (v->key == u->key) {
        if (v->rank < u->rank) {
            parent = v;
            child = u;
        }
    }

    child->next = parent->child;
    parent->child = child;
    child->second_parent = nullptr;
    return parent;
}

HollowHeapNode* HollowHeap::insert(long long key, int value) {
    HollowHeapNode* handle = make_handle();
    HollowHeapCell* cell = make_cell(key, value, handle);
    handle->cell = cell;
    active_size_++;
    root_ = link(root_, cell);
    return handle;
}

void HollowHeap::decrease_key(HollowHeapNode* handle, long long new_key) {
    if (!handle || !handle->cell) {
        throw std::invalid_argument("handle is null");
    }
    HollowHeapCell* node = handle->cell;
    if (new_key > node->key) {
        throw std::invalid_argument("new_key is greater than current key");
    }

    if (node == root_) {
        node->key = new_key;
        return;
    }

    HollowHeapCell* new_cell = make_cell(new_key, node->value, handle);
    handle->cell = new_cell;
    if (node->rank > 2) {
        new_cell->rank = node->rank - 2;
    }
    node->hollow = true;

    if (!root_) {
        root_ = new_cell;
        return;
    }

    HollowHeapCell* old_root = root_;
    root_ = link(root_, new_cell);
    if (root_ == old_root) {
        new_cell->child = node;
        node->second_parent = new_cell;
    }
}

std::pair<long long, int> HollowHeap::extract_min() {
    if (!root_) {
        throw std::runtime_error("extract_min from empty HollowHeap");
    }

    HollowHeapCell* old_root = root_;
    std::pair<long long, int> result(old_root->key, old_root->value);
    if (old_root->owner && old_root->owner->cell == old_root) {
        old_root->owner->cell = nullptr;
    }

    to_delete_.clear();
    to_delete_.push_back(old_root);

    int max_rank = -1;

    for (std::size_t idx = 0; idx < to_delete_.size(); ++idx) {
        HollowHeapCell* parent = to_delete_[idx];
        HollowHeapCell* cur = parent->child;
        parent->child = nullptr;

        while (cur) {
            HollowHeapCell* next = cur->next;
            cur->next = nullptr;

            if (!cur->hollow) {
                while (true) {
                    ensure_rank_capacity(cur->rank);
                    HollowHeapCell*& slot = rankmap_[cur->rank];
                    if (!slot) break;
                    HollowHeapCell* other = slot;
                    slot = nullptr;
                    cur = link(cur, other);
                    cur->rank++;
                }
                ensure_rank_capacity(cur->rank);
                rankmap_[cur->rank] = cur;
                if (static_cast<int>(cur->rank) > max_rank) {
                    max_rank = static_cast<int>(cur->rank);
                }
            } else {
                if (!cur->second_parent) {
                    to_delete_.push_back(cur);
                } else {
                    if (cur->second_parent == parent) {
                        cur->second_parent = nullptr;
                        break;
                    } else {
                        cur->second_parent = nullptr;
                        cur->next = nullptr;
                    }
                }
            }

            cur = next;
        }
    }

    root_ = nullptr;
    if (max_rank >= 0) {
        for (int i = max_rank; i >= 0; --i) {
            if (i >= static_cast<int>(rankmap_.size())) continue;
            HollowHeapCell* node = rankmap_[i];
            if (!node) continue;
            if (!root_) {
                root_ = node;
            } else {
                root_ = link(root_, node);
            }
            rankmap_[i] = nullptr;
        }
    }

    active_size_--;
    old_root->hollow = true;
    if (!root_) {
        clear_rankmap();
    }

    return result;
}

std::pair<long long, int> HollowHeap::peek_min() const {
    if (!root_) {
        throw std::runtime_error("peek_min from empty HollowHeap");
    }
    return {root_->key, root_->value};
}

void HollowHeap::merge(PriorityQueue<HollowHeapNode>& other_base) {
    auto* other = dynamic_cast<HollowHeap*>(&other_base);
    if (!other) throw std::invalid_argument("merge requires another HollowHeap");
    if (other == this || other->active_size_ == 0) return;

    handles_.reserve(handles_.size() + other->handles_.size());
    for (auto& handle : other->handles_) {
        handles_.push_back(std::move(handle));
    }
    other->handles_.clear();

    cells_.reserve(cells_.size() + other->cells_.size());
    for (auto& cell : other->cells_) {
        cells_.push_back(std::move(cell));
    }
    other->cells_.clear();

    if (!root_) {
        root_ = other->root_;
    } else if (other->root_) {
        root_ = link(root_, other->root_);
    }

    active_size_ += other->active_size_;
    other->active_size_ = 0;
    other->root_ = nullptr;
    other->rankmap_.assign(kInitialRankCapacity, nullptr);
    other->to_delete_.clear();
}