#pragma once

#include <cstddef>

// Aggregated structural properties recorded per heap while it is in use.
struct HeapStructureStats {
    std::size_t current_nodes = 0;        // live nodes currently stored
    std::size_t max_nodes = 0;            // peak node count during the run
    std::size_t max_tree_height = 0;      // tallest tree level count observed
    std::size_t max_roots = 0;            // maximum concurrent roots (or root candidates)
    std::size_t consolidation_passes = 0; // how often restructuring/consolidation ran
    std::size_t link_operations = 0;      // number of tree linking/swap actions performed
    std::size_t current_bytes = 0;        // approximate memory footprint (bytes)
    std::size_t max_bytes = 0;            // peak bytes
};
