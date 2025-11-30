#pragma once
#include <utility>

template <typename Nodetype>
class PriorityQueue
{
public:
    virtual Nodetype* insert(long long key, int value) = 0;
    virtual std::pair<long long, int> extract_min() = 0;
    virtual std::pair<long long, int> peek_min() const = 0;
    virtual void decrease_key(Nodetype* node, long long new_key) = 0;
    virtual void merge(PriorityQueue& other) = 0;
    virtual bool is_empty() const = 0;
    virtual ~PriorityQueue() = default;
};
