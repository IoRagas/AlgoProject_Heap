#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <system_error>
#include <string>
#include <utility>
#include <vector>

#include "Dijkstra.h"
#include "Graph.h"

namespace {
struct DatasetOption {
    std::string name;
    std::string path;
};

constexpr long long kInfinity = std::numeric_limits<long long>::max() / 4;

int read_int_with_default(const std::string& prompt, int default_value) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return default_value;
    }
    if (line.empty()) {
        return default_value;
    }
    try {
        return std::stoi(line);
    } catch (...) {
        return default_value;
    }
}

std::string read_line_with_default(const std::string& prompt, const std::string& default_value) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line) || line.empty()) {
        return default_value;
    }
    return line;
}

void print_distance_sample(const std::vector<long long>& distances, int sample_count) {
    int printed = 0;
    for (std::size_t node = 0; node < distances.size() && printed < sample_count; ++node) {
        long long dist = distances[node];
        if (dist >= kInfinity) {
            continue;
        }
        std::cout << "  Node " << node << " -> distance " << dist << '\n';
        ++printed;
    }
    if (printed == 0) {
        std::cout << "  No reachable nodes to display." << std::endl;
    }
}

double average_us(long long ns_total, std::size_t count) {
    if (count == 0) return 0.0;
    return (static_cast<double>(ns_total) / static_cast<double>(count)) / 1000.0;
}

std::string heap_name(HeapSelection selection) {
    switch (selection) {
        case HeapSelection::kBinary: return "Binary";
        case HeapSelection::kFibonacci: return "Fibonacci";
        case HeapSelection::kHollow: return "Hollow";
        default: return "Unknown";
    }
}

struct RunSummary {
    HeapSelection heap;
    long long elapsed_ms = 0;
    std::size_t reachable_nodes = 0;
    int farthest_node = -1;
    long long farthest_distance = 0;
    QueueMetrics metrics;
};

RunSummary execute_run(const Graph& graph, int source, HeapSelection selection, DijkstraResult* out_result) {
    auto queue = make_queue_adapter(selection);
    auto start = std::chrono::steady_clock::now();
    DijkstraResult result = run_dijkstra(graph, source, *queue);
    auto finish = std::chrono::steady_clock::now();

    RunSummary summary;
    summary.heap = selection;
    summary.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    summary.metrics = result.metrics;

    for (std::size_t node = 0; node < result.distances.size(); ++node) {
        long long dist = result.distances[node];
        if (dist >= kInfinity) {
            continue;
        }
        summary.reachable_nodes++;
        if (dist > summary.farthest_distance) {
            summary.farthest_distance = dist;
            summary.farthest_node = static_cast<int>(node);
        }
    }

    if (out_result) {
        *out_result = std::move(result);
    }

    return summary;
}

std::string sanitize_filename_component(const std::string& value) {
    std::string sanitized;
    sanitized.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch) || ch == '-' || ch == '_') {
            sanitized.push_back(static_cast<char>(ch));
        } else {
            sanitized.push_back('_');
        }
    }
    return sanitized;
}

std::filesystem::path default_summary_path(const DatasetOption& dataset, int source) {
    std::ostringstream oss;
    oss << sanitize_filename_component(dataset.name) << "_src" << source << "_summary.txt";
    return std::filesystem::path("Results") / oss.str();
}

std::string format_summary_table(const std::vector<RunSummary>& runs, const std::string& dataset_name) {
    std::ostringstream oss;
    oss << "=== Batch Summary for " << dataset_name << " ===\n";
    oss << std::left << std::setw(12) << "Heap" << std::right
        << std::setw(14) << "Runtime(ms)"
        << std::setw(14) << "Inserts"
        << std::setw(18) << "Insert Avg (us)"
        << std::setw(14) << "Extracts"
        << std::setw(18) << "Extract Avg (us)"
        << std::setw(14) << "Decreases"
        << std::setw(20) << "Decrease Avg (us)"
        << std::setw(14) << "Reachable"
        << '\n';

    oss << std::string(138, '-') << '\n';
    oss << std::fixed << std::setprecision(3);
    for (const auto& run : runs) {
        double insert_avg = average_us(run.metrics.insert_time_ns, run.metrics.insert_count);
        double extract_avg = average_us(run.metrics.extract_time_ns, run.metrics.extract_count);
        double decrease_avg = average_us(run.metrics.decrease_time_ns, run.metrics.decrease_count);
        oss << std::left << std::setw(12) << heap_name(run.heap) << std::right
            << std::setw(14) << run.elapsed_ms
            << std::setw(14) << run.metrics.insert_count
            << std::setw(18) << insert_avg
            << std::setw(14) << run.metrics.extract_count
            << std::setw(18) << extract_avg
            << std::setw(14) << run.metrics.decrease_count
            << std::setw(20) << decrease_avg
            << std::setw(14) << run.reachable_nodes
            << '\n';
    }
    oss.unsetf(std::ios::floatfield);
    return oss.str();
}

void write_summary_report(const std::string& report, const std::filesystem::path& out_path) {
    if (!out_path.empty()) {
        if (out_path.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(out_path.parent_path(), ec);
        }
        std::ofstream file(out_path);
        if (!file) {
            std::cerr << "Warning: failed to write summary file at " << out_path << std::endl;
        } else {
            file << report;
        }
    }
}
} // namespace

int main() try {
    const std::vector<DatasetOption> datasets = {
        {"Chongqing road network", "Data/Chongqing.road-d"},
        {"Hong Kong road network", "Data/Hongkong.road-d"},
        {"Shanghai road network", "Data/Shanghai.road-d"},
    };

    std::cout << "=== Dijkstra Benchmark Driver ===\n";
    std::cout << "Available datasets:" << std::endl;
    for (std::size_t i = 0; i < datasets.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << datasets[i].name << " (" << datasets[i].path << ")" << std::endl;
    }

    int dataset_choice = read_int_with_default("Select dataset [default: 1]: ", 1);
    if (dataset_choice < 1 || dataset_choice > static_cast<int>(datasets.size())) {
        std::cout << "Invalid selection. Falling back to option 1." << std::endl;
        dataset_choice = 1;
    }

    Graph graph;
    std::string load_error;
    const auto& dataset = datasets[static_cast<std::size_t>(dataset_choice - 1)];
    std::cout << "Loading " << dataset.name << "..." << std::endl;
    if (!graph.load_from_file(dataset.path, &load_error)) {
        std::cerr << "Failed to load dataset: " << load_error << std::endl;
        return 1;
    }

    std::cout << "Loaded graph with " << graph.node_count() << " nodes and " << graph.edge_count() << " edges." << std::endl;

    int source = read_int_with_default("Enter source vertex id [default: 0]: ", 0);
    if (source < 0 || static_cast<std::size_t>(source) >= graph.node_count()) {
        std::cout << "Source out of range. Using 0." << std::endl;
        source = 0;
    }

    std::cout << "Select run mode:" << std::endl;
    std::cout << "  [1] Single run (interactive)" << std::endl;
    std::cout << "  [2] Run all heaps and produce summary" << std::endl;
    int mode_choice = read_int_with_default("Mode [default: 1]: ", 1);

    if (mode_choice == 2) {
        std::vector<RunSummary> summaries;
        for (HeapSelection selection : {HeapSelection::kBinary, HeapSelection::kFibonacci, HeapSelection::kHollow}) {
            std::cout << "Running " << heap_name(selection) << " heap..." << std::flush;
            RunSummary summary = execute_run(graph, source, selection, nullptr);
            std::cout << " done (" << summary.elapsed_ms << " ms)." << std::endl;
            summaries.push_back(summary);
        }

        auto default_path = default_summary_path(dataset, source);
        std::string out_path_input = read_line_with_default(
            "Enter summary file path [default: " + default_path.string() + "]: ",
            default_path.string());
        std::filesystem::path out_path(out_path_input);

        std::string report = format_summary_table(summaries, dataset.name);
        std::cout << '\n' << report << std::endl;
        write_summary_report(report, out_path);
        std::cout << "Summary written to " << out_path << std::endl;
        return 0;
    }

    std::cout << "Select heap implementation:" << std::endl;
    std::cout << "  [1] Binary Heap" << std::endl;
    std::cout << "  [2] Fibonacci Heap" << std::endl;
    std::cout << "  [3] Hollow Heap" << std::endl;
    int heap_choice = read_int_with_default("Choice [default: 3]: ", 3);
    if (heap_choice < 1 || heap_choice > 3) {
        std::cout << "Invalid selection. Using Hollow Heap." << std::endl;
        heap_choice = 3;
    }

    HeapSelection selection = static_cast<HeapSelection>(heap_choice);
    DijkstraResult result;
    RunSummary summary = execute_run(graph, source, selection, &result);

    std::cout << "\n--- Run Summary ---" << std::endl;
    std::cout << "Heap type      : " << heap_name(selection) << std::endl;
    std::cout << "Source vertex  : " << source << std::endl;
    std::cout << "Reachable nodes: " << summary.reachable_nodes << " / " << graph.node_count() << std::endl;
    if (summary.farthest_node >= 0) {
        std::cout << "Farthest node  : " << summary.farthest_node << " @ distance " << summary.farthest_distance << std::endl;
    }
    std::cout << "Elapsed time   : " << summary.elapsed_ms << " ms" << std::endl;

    const auto& metrics = summary.metrics;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Insert ops   : " << metrics.insert_count
              << " avg " << average_us(metrics.insert_time_ns, metrics.insert_count) << " us" << std::endl;
    std::cout << "Decrease ops : " << metrics.decrease_count
              << " avg " << average_us(metrics.decrease_time_ns, metrics.decrease_count) << " us" << std::endl;
    std::cout << "Extract ops  : " << metrics.extract_count
              << " avg " << average_us(metrics.extract_time_ns, metrics.extract_count) << " us" << std::endl;

    std::cout.unsetf(std::ios::floatfield);
    std::cout << std::setprecision(6);

    std::cout << "\nSample distances (first 10 reachable nodes):" << std::endl;
    print_distance_sample(result.distances, 10);

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}
catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
}
