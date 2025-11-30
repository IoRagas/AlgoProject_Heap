#pragma once

#include "PriorityQueue.h"
#include <cstddef>
#include <utility>
#include <vector>
#include <memory>

struct HollowHeapCell;

struct HollowHeapNode {
	HollowHeapCell* cell = nullptr;
};

class HollowHeap : public PriorityQueue<HollowHeapNode>
{
public:
	HollowHeap();
	~HollowHeap() override;

	HollowHeapNode* insert(long long key, int value) override;
	std::pair<long long, int> extract_min() override;
	std::pair<long long, int> peek_min() const override;
	void decrease_key(HollowHeapNode* node, long long new_key) override;
	void merge(PriorityQueue& other) override;
	bool is_empty() const override;

private:
	HollowHeapNode* make_handle();
	HollowHeapCell* make_cell(long long key, int value, HollowHeapNode* owner);
	HollowHeapCell* link(HollowHeapCell* u, HollowHeapCell* v);
	void ensure_rank_capacity(std::size_t rank);
	void clear_rankmap();

	HollowHeapCell* root_;
	std::size_t active_size_;

	std::vector<std::unique_ptr<HollowHeapNode>> handles_;
	std::vector<std::unique_ptr<HollowHeapCell>> cells_;
	std::vector<HollowHeapCell*> rankmap_;
	std::vector<HollowHeapCell*> to_delete_;
};
