/*#include <iostream>
#include <vector>
#include <cstdio>
#include <string>

#include "Graph.h"
#include "Dijkstra.h"
#include "BinaryHeap.h"
#include "FibonacciHeap.h"
#include "HollowHeap.h"

namespace {
std::string make_temp_graph() {
    const char* path = "test_small.road-d";
    FILE* tmp = std::fopen(path, "w");
    if (!tmp) {
        throw std::runtime_error("Failed to create temp graph file");
    }

    std::fprintf(tmp, "0 1 4\n");
    std::fprintf(tmp, "0 2 1\n");
    std::fprintf(tmp, "2 1 2\n");
    std::fprintf(tmp, "1 3 1\n");
    std::fprintf(tmp, "2 3 5\n");
    std::fprintf(tmp, "3 4 3\n");
    std::fprintf(tmp, "4 5 1\n");
    std::fprintf(tmp, "5 6 2\n");
    std::fprintf(tmp, "6 7 2\n");
    std::fprintf(tmp, "7 8 2\n");
    std::fprintf(tmp, "8 9 2\n");
    std::fprintf(tmp, "0 9 20\n");
    std::fprintf(tmp, "2 5 10\n");
    std::fclose(tmp);
    return path;
}

void print_results(const char* heap_name,
                   const std::vector<long long>& dist,
                   const std::vector<long long>& expected) {
    std::cout << "Testing " << heap_name << "...\n";
    bool pass = true;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        long long got = i < dist.size() ? dist[i] : -1;
        std::cout << "  Node " << i << ": " << got;
        if (got != expected[i]) {
            std::cout << " [expected " << expected[i] << "]";
            pass = false;
        } else {
            std::cout << " [OK]";
        }
        std::cout << '\n';
    }
    std::cout << (pass ? ">> PASS\n\n" : ">> FAIL\n\n");
}
}

int main() try {
    const std::string graph_path = make_temp_graph();
    Graph graph;
    if (!graph.load_from_file(graph_path)) {
        std::cerr << "Failed to load temp graph" << std::endl;
        return 1;
    }

    const std::vector<long long> expected{0, 3, 1, 4, 7, 8, 10, 12, 14, 16};
    std::cout << "=== Dijkstra correctness test (10 nodes) ===\n";

    {
        auto result = run_dijkstra(graph, 0, make_queue_adapter<BinaryHeap>());
        print_results("BinaryHeap", result.dist, expected);
    }
    {
        auto result = run_dijkstra(graph, 0, make_queue_adapter<FibonacciHeap>());
        print_results("FibonacciHeap", result.dist, expected);
    }
    {
        auto result = run_dijkstra(graph, 0, make_queue_adapter<HollowHeap>());
        print_results("HollowHeap", result.dist, expected);
    }

    std::remove(graph_path.c_str());
    std::cout << "Test complete." << std::endl;
    return 0;
} catch (const std::exception& ex) {
    std::cerr << "Test failed: " << ex.what() << std::endl;
    return 1;
}
*/