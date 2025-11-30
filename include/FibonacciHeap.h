#pragma once

#include "PriorityQueue.h"
#include <cstddef>

struct FibonacciHeapNode {
	long long key;
	int value;
	FibonacciHeapNode* parent;
	FibonacciHeapNode* child;
	FibonacciHeapNode* left;
	FibonacciHeapNode* right;
	int degree;
	bool mark;
};

class FibonacciHeap : public PriorityQueue<FibonacciHeapNode>
{
public:
	FibonacciHeap();
	~FibonacciHeap() override;

	FibonacciHeapNode* insert(long long key, int value) override;
	std::pair<long long, int> extract_min() override;
	std::pair<long long, int> peek_min() const override;
	void decrease_key(FibonacciHeapNode* node, long long new_key) override;
	void merge(PriorityQueue& other) override;
	bool is_empty() const override;

private:
	FibonacciHeapNode* min_;
	std::size_t size_;

	void add_to_root_list(FibonacciHeapNode* node);
	void remove_from_root_list(FibonacciHeapNode* node);
	void consolidate();
	void link_nodes(FibonacciHeapNode* child, FibonacciHeapNode* parent);
	void cut(FibonacciHeapNode* node, FibonacciHeapNode* parent);
	void cascading_cut(FibonacciHeapNode* node);
	void delete_all(FibonacciHeapNode* node);
};
