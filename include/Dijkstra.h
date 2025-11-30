#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "Graph.h"
#include "HeapStats.h"

struct QueueMetrics {
    std::size_t insert_count = 0;
    std::size_t decrease_count = 0;
    std::size_t extract_count = 0;
    long long insert_time_ns = 0;
    long long decrease_time_ns = 0;
    long long extract_time_ns = 0;
};

struct DijkstraResult {
    std::vector<long long> distances;
    std::vector<int> parents;
    QueueMetrics metrics;
    HeapStructureStats structure;
};

enum class HeapSelection {
    kBinary = 1,
    kFibonacci = 2,
    kHollow = 3
};

class DijkstraQueue {
public:
    virtual ~DijkstraQueue() = default;
    virtual void reset(std::size_t node_count) = 0;
    virtual void push_or_decrease(int vertex, long long key) = 0;
    virtual std::pair<long long, int> extract_min() = 0;
    virtual bool empty() const = 0;
    virtual const QueueMetrics& metrics() const = 0;
    virtual const HeapStructureStats& structure_stats() const = 0;
};

std::unique_ptr<DijkstraQueue> make_queue_adapter(HeapSelection selection);

DijkstraResult run_dijkstra(const Graph& graph, int source, DijkstraQueue& queue);
