#include "Dijkstra.h"

#include "BinaryHeap.h"
#include "FibonacciHeap.h"
#include "HollowHeap.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
constexpr long long kInfinity = std::numeric_limits<long long>::max() / 4;

template <typename HeapType, typename HandleType>
class HeapAdapter : public DijkstraQueue {
public:
    using Clock = std::chrono::steady_clock;

    HeapAdapter() : heap_(std::make_unique<HeapType>()) {}

    void reset(std::size_t node_count) override {
        heap_ = std::make_unique<HeapType>();
        handles_.assign(node_count, nullptr);
        metrics_ = {};
    }

    void push_or_decrease(int vertex, long long key) override {
        if (vertex < 0) {
            throw std::invalid_argument("vertex must be non-negative");
        }
        if (static_cast<std::size_t>(vertex) >= handles_.size()) {
            handles_.resize(static_cast<std::size_t>(vertex) + 1, nullptr);
        }

        auto*& handle = handles_[static_cast<std::size_t>(vertex)];
        if (!handle) {
            const auto start = Clock::now();
            handle = heap_->insert(key, vertex);
            const auto end = Clock::now();
            metrics_.insert_count++;
            metrics_.insert_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        } else {
            const auto start = Clock::now();
            heap_->decrease_key(handle, key);
            const auto end = Clock::now();
            metrics_.decrease_count++;
            metrics_.decrease_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }
    }

    std::pair<long long, int> extract_min() override {
        const auto start = Clock::now();
        auto result = heap_->extract_min();
        const auto end = Clock::now();
        metrics_.extract_count++;
        metrics_.extract_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        int vertex = result.second;
        if (vertex >= 0 && static_cast<std::size_t>(vertex) < handles_.size()) {
            handles_[static_cast<std::size_t>(vertex)] = nullptr;
        }
        return result;
    }

    bool empty() const override {
        return heap_->is_empty();
    }

    const QueueMetrics& metrics() const override { return metrics_; }

    const HeapStructureStats& structure_stats() const override {
        return heap_->structure_stats();
    }

private:
    std::unique_ptr<HeapType> heap_;
    std::vector<HandleType*> handles_;
    QueueMetrics metrics_;
};
} // namespace

std::unique_ptr<DijkstraQueue> make_queue_adapter(HeapSelection selection) {
    switch (selection) {
        case HeapSelection::kBinary:
            return std::make_unique<HeapAdapter<BinaryHeap, BinaryHeapNode>>();
        case HeapSelection::kFibonacci:
            return std::make_unique<HeapAdapter<FibonacciHeap, FibonacciHeapNode>>();
        case HeapSelection::kHollow:
            return std::make_unique<HeapAdapter<HollowHeap, HollowHeapNode>>();
        default:
            throw std::invalid_argument("Unknown heap selection");
    }
}

DijkstraResult run_dijkstra(const Graph& graph, int source, DijkstraQueue& queue) {
    if (graph.empty()) {
        throw std::invalid_argument("Graph is empty");
    }
    if (source < 0 || static_cast<std::size_t>(source) >= graph.node_count()) {
        throw std::out_of_range("Source vertex out of range");
    }

    const std::size_t n = graph.node_count();
    DijkstraResult result;
    result.distances.assign(n, kInfinity);
    result.parents.assign(n, -1);

    queue.reset(n);
    result.distances[static_cast<std::size_t>(source)] = 0;
    queue.push_or_decrease(source, 0);

    while (!queue.empty()) {
        auto [dist_u, u] = queue.extract_min();
        if (dist_u > result.distances[static_cast<std::size_t>(u)]) {
            continue;
        }

        for (const auto& edge : graph.neighbors(u)) {
            if (edge.weight >= kInfinity) {
                continue;
            }
            long long capped = kInfinity - edge.weight;
            if (dist_u > capped) {
                continue;
            }
            long long candidate = dist_u + edge.weight;
            auto& current = result.distances[static_cast<std::size_t>(edge.to)];
            if (candidate < current) {
                current = candidate;
                result.parents[static_cast<std::size_t>(edge.to)] = u;
                queue.push_or_decrease(edge.to, candidate);
            }
        }
    }

    result.metrics = queue.metrics();
    result.structure = queue.structure_stats();
    return result;
}
