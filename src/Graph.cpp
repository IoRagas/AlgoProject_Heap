#include "Graph.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

namespace {
constexpr long long kMaxWeight = std::numeric_limits<long long>::max() / 4;
}

bool Graph::load_from_file(const std::string& path, std::string* error_message) {
    std::ifstream input(path);
    if (!input) {
        if (error_message) {
            *error_message = "Failed to open graph file: " + path;
        }
        return false;
    }

    reset();

    std::string line;
    std::size_t line_number = 0;
    int max_node_id = -1;
    while (std::getline(input, line)) {
        ++line_number;
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        int from = -1;
        int to = -1;
        double weight = 0.0;
        if (!(iss >> from >> to >> weight)) {
            if (error_message) {
                std::ostringstream oss;
                oss << "Failed to parse line " << line_number << ": " << line;
                *error_message = oss.str();
            }
            return false;
        }

        if (from < 0 || to < 0) {
            if (error_message) {
                std::ostringstream oss;
                oss << "Encountered negative node id on line " << line_number;
                *error_message = oss.str();
            }
            return false;
        }

        long long discrete_weight = static_cast<long long>(std::llround(weight));
        if (discrete_weight < 0 || discrete_weight > kMaxWeight) {
            if (error_message) {
                std::ostringstream oss;
                oss << "Weight out of range on line " << line_number;
                *error_message = oss.str();
            }
            return false;
        }

        int needed = std::max(from, to);
        if (needed >= static_cast<int>(adjacency_.size())) {
            adjacency_.resize(static_cast<std::size_t>(needed) + 1);
        }

        adjacency_[static_cast<std::size_t>(from)].push_back(GraphEdge{to, discrete_weight});
        ++edge_count_;

        if (from > max_node_id) max_node_id = from;
        if (to > max_node_id) max_node_id = to;
    }

    if (adjacency_.empty()) {
        if (error_message) {
            *error_message = "Graph file contains no edges.";
        }
        return false;
    }

    if (static_cast<std::size_t>(max_node_id + 1) > adjacency_.size()) {
        adjacency_.resize(static_cast<std::size_t>(max_node_id) + 1);
    }

    return true;
}

void Graph::reset() {
    adjacency_.clear();
    edge_count_ = 0;
}
