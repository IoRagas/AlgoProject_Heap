#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct GraphEdge {
    int to = -1;
    long long weight = 0; // weight stored in whole meters
};

class Graph {
public:
    bool load_from_file(const std::string& path, std::string* error_message = nullptr);

    std::size_t node_count() const noexcept { return adjacency_.size(); }
    std::size_t edge_count() const noexcept { return edge_count_; }
    bool empty() const noexcept { return adjacency_.empty(); }

    const std::vector<GraphEdge>& neighbors(int node) const { return adjacency_[static_cast<std::size_t>(node)]; }

private:
    void reset();

    std::vector<std::vector<GraphEdge>> adjacency_;
    std::size_t edge_count_ = 0;
};
