#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
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
#include <random>

#include "BinaryHeap.h"
#include "Dijkstra.h"
#include "FibonacciHeap.h"
#include "Graph.h"
#include "HollowHeap.h"

namespace {
struct DatasetOption {
    std::string name;
    std::string path;
};

constexpr long long kInfinity = std::numeric_limits<long long>::max() / 4;

void print_section_header(const std::string& title) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "» " << title << "\n";
    std::cout << std::string(80, '=') << std::endl;
}

void print_subsection_header(const std::string& title) {
    std::cout << "\n" << std::string(60, '-') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(60, '-') << std::endl;
}


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

bool prompt_yes_no(const std::string& prompt, bool default_value) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line) || line.empty()) {
        return default_value;
    }
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
    if (c == 'y' || c == '1') return true;
    if (c == 'n' || c == '0') return false;
    return default_value;
}

void print_distance_sample(const std::vector<long long>& distances, int sample_count) {
    const bool print_all = sample_count <= 0;
    int printed = 0;
    for (std::size_t node = 0; node < distances.size(); ++node) {
        long long dist = distances[node];
        if (dist >= kInfinity) {
            continue;
        }
        std::cout << "  Node " << node << " -> distance " << dist << '\n';
        ++printed;
        if (!print_all && printed >= sample_count) {
            break;
        }
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
    HeapStructureStats structure;
};

struct WorkloadStats {
    HeapSelection heap;
    std::size_t operations = 0;
    QueueMetrics metrics;
    long long total_runtime_ms = 0;
    HeapStructureStats structure;
};

struct WorkloadMix {
    int insert_pct = 40;
    int decrease_pct = 35;
    int extract_pct = 25;

    bool valid() const {
        if (insert_pct < 0 || decrease_pct < 0 || extract_pct < 0) {
            return false;
        }
        return insert_pct + decrease_pct + extract_pct == 100;
    }
};

struct AggregateStats {
    HeapSelection heap;
    std::size_t runs = 0;
    long long total_runtime_ms = 0;
    long long max_runtime_ms = 0;
    std::size_t total_reachable = 0;
    std::size_t max_reachable = 0;
    QueueMetrics total_metrics;
    HeapStructureStats structure;
};

void accumulate_structure_stats(HeapStructureStats& dest, const HeapStructureStats& src) {
    dest.max_nodes = std::max(dest.max_nodes, src.max_nodes);
    dest.max_tree_height = std::max(dest.max_tree_height, src.max_tree_height);
    dest.max_roots = std::max(dest.max_roots, src.max_roots);
    dest.max_bytes = std::max(dest.max_bytes, src.max_bytes);
    dest.consolidation_passes = std::max(dest.consolidation_passes, src.consolidation_passes);
    dest.link_operations = std::max(dest.link_operations, src.link_operations);
}

void accumulate_metrics(QueueMetrics& dest, const QueueMetrics& src) {
    dest.insert_count += src.insert_count;
    dest.decrease_count += src.decrease_count;
    dest.extract_count += src.extract_count;
    dest.insert_time_ns += src.insert_time_ns;
    dest.decrease_time_ns += src.decrease_time_ns;
    dest.extract_time_ns += src.extract_time_ns;
}

void accumulate_aggregate(AggregateStats& agg, const RunSummary& summary) {
    agg.runs++;
    agg.total_runtime_ms += summary.elapsed_ms;
    agg.max_runtime_ms = std::max(agg.max_runtime_ms, summary.elapsed_ms);
    agg.total_reachable += summary.reachable_nodes;
    agg.max_reachable = std::max(agg.max_reachable, summary.reachable_nodes);
    accumulate_metrics(agg.total_metrics, summary.metrics);
    accumulate_structure_stats(agg.structure, summary.structure);
}

std::string format_all_sources_table(const std::vector<AggregateStats>& aggregates,
                                     const std::string& dataset_name,
                                     std::size_t source_count) {
    if (aggregates.empty()) {
        return {};
    }

    std::ostringstream oss;
    oss << "=== All-Sources Summary for " << dataset_name << " (" << source_count << " sources) ===\n";
    oss << std::left << std::setw(12) << "Heap" << std::right
        << std::setw(12) << "Runs"
        << std::setw(16) << "AvgRuntime(ms)"
        << std::setw(16) << "MaxRuntime(ms)"
        << std::setw(16) << "TotalRuntime(s)"
        << std::setw(16) << "AvgReachable"
        << std::setw(18) << "Insert Avg (us)"
        << std::setw(18) << "Extract Avg (us)"
        << std::setw(18) << "Decrease Avg (us)" << '\n';
    oss << std::string(150, '-') << '\n';
    oss << std::fixed << std::setprecision(3);
    for (const auto& agg : aggregates) {
        if (agg.runs == 0) {
            continue;
        }
        double avg_runtime_ms = static_cast<double>(agg.total_runtime_ms) / static_cast<double>(agg.runs);
        double total_runtime_s = static_cast<double>(agg.total_runtime_ms) / 1000.0;
        double avg_reachable = static_cast<double>(agg.total_reachable) / static_cast<double>(agg.runs);
        double insert_avg = average_us(agg.total_metrics.insert_time_ns, agg.total_metrics.insert_count);
        double extract_avg = average_us(agg.total_metrics.extract_time_ns, agg.total_metrics.extract_count);
        double decrease_avg = average_us(agg.total_metrics.decrease_time_ns, agg.total_metrics.decrease_count);
        oss << std::left << std::setw(12) << heap_name(agg.heap) << std::right
            << std::setw(12) << agg.runs
            << std::setw(16) << avg_runtime_ms
            << std::setw(16) << agg.max_runtime_ms
            << std::setw(16) << total_runtime_s
            << std::setw(16) << avg_reachable
            << std::setw(18) << insert_avg
            << std::setw(18) << extract_avg
            << std::setw(18) << decrease_avg
            << '\n';
    }
    oss.unsetf(std::ios::floatfield);
    return oss.str();
}

RunSummary execute_run(const Graph& graph, int source, HeapSelection selection, DijkstraResult* out_result) {
    auto queue = make_queue_adapter(selection);
    auto start = std::chrono::steady_clock::now();
    DijkstraResult result = run_dijkstra(graph, source, *queue);
    auto finish = std::chrono::steady_clock::now();

    RunSummary summary;
    summary.heap = selection;
    summary.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    summary.metrics = result.metrics;
    summary.structure = result.structure;

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

std::filesystem::path default_workload_path(std::size_t operations) {
    std::ostringstream oss;
    oss << "RandomPQ_ops" << operations << "_summary.txt";
    return std::filesystem::path("Results") / oss.str();
}

std::filesystem::path default_all_sources_path(const DatasetOption& dataset, std::size_t start_source, std::size_t count) {
    std::ostringstream oss;
    oss << sanitize_filename_component(dataset.name) << "_all_sources_start" << start_source
        << "_count" << count << ".txt";
    return std::filesystem::path("Results") / oss.str();
}

std::filesystem::path detect_exe_directory(char** argv) {
    if (!argv || !argv[0]) {
        return std::filesystem::current_path();
    }
    std::error_code ec;
    auto exe_path = std::filesystem::canonical(argv[0], ec);
    if (ec) {
        return std::filesystem::current_path();
    }
    return exe_path.parent_path();
}

std::filesystem::path resolve_dataset_path(const std::string& relative, const std::filesystem::path& exe_dir) {
    std::filesystem::path rel_path(relative);
    if (rel_path.is_absolute()) {
        std::error_code ec;
        if (std::filesystem::exists(rel_path, ec)) {
            return rel_path;
        }
        return {};
    }

    std::vector<std::filesystem::path> bases;
    bases.emplace_back(); // current relative path
    bases.push_back(std::filesystem::current_path());
    bases.push_back(exe_dir);
    auto parent = exe_dir;
    for (int i = 0; i < 4 && !parent.empty(); ++i) {
        parent = parent.parent_path();
        if (!parent.empty()) {
            bases.push_back(parent);
        }
    }

    for (const auto& base : bases) {
        std::filesystem::path candidate = base.empty() ? rel_path : (base / rel_path);
        std::error_code ec;
        if (candidate.empty()) {
            continue;
        }
        if (std::filesystem::exists(candidate, ec)) {
            auto resolved = std::filesystem::canonical(candidate, ec);
            return ec ? candidate : resolved;
        }
    }
    return {};
}

std::string format_bytes_with_mb(std::size_t bytes) {
    std::ostringstream oss;
    oss << bytes << " (" << std::fixed << std::setprecision(2)
        << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MB)";
    oss.unsetf(std::ios::floatfield);
    return oss.str();
}

template <typename Collection, typename LabelAccessor>
std::string format_structure_table(const Collection& items, const std::string& title, LabelAccessor label_accessor) {
    if (items.empty()) {
        return {};
    }

    std::ostringstream oss;
    oss << title << '\n';
    oss << std::left << std::setw(12) << "Heap" << std::right
        << std::setw(12) << "MaxNodes"
        << std::setw(12) << "MaxBytes"
        << std::setw(12) << "MaxMB"
        << std::setw(10) << "Height"
        << std::setw(10) << "MaxRoots"
        << std::setw(16) << "ConsolPasses"
        << std::setw(12) << "LinkOps" << '\n';
    oss << std::string(96, '-') << '\n';
    for (const auto& item : items) {
        const auto& stats = item.structure;
        const double max_mb = stats.max_bytes / (1024.0 * 1024.0);
        oss << std::left << std::setw(12) << label_accessor(item) << std::right
            << std::setw(12) << stats.max_nodes
            << std::setw(12) << stats.max_bytes;
        oss << std::setw(12) << std::fixed << std::setprecision(3) << max_mb;
        oss.unsetf(std::ios::floatfield);
        oss << std::setprecision(6);
        oss << std::setw(10) << stats.max_tree_height
            << std::setw(10) << stats.max_roots
            << std::setw(16) << stats.consolidation_passes
            << std::setw(12) << stats.link_operations
            << '\n';
    }
    return oss.str();
}

void print_structure_metrics(const HeapStructureStats& stats) {
    std::cout << "Max nodes      : " << stats.max_nodes << std::endl;
    std::cout << "Max bytes      : " << format_bytes_with_mb(stats.max_bytes) << std::endl;
    std::cout << "Max tree height: " << stats.max_tree_height << std::endl;
    std::cout << "Max roots      : " << stats.max_roots << std::endl;
    std::cout << "Consolidations : " << stats.consolidation_passes << std::endl;
    std::cout << "Link operations: " << stats.link_operations << std::endl;
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
    std::string report = oss.str();
    std::string structure_section = format_structure_table(
        runs,
        "=== Structural Metrics for " + dataset_name + " ===",
        [](const RunSummary& run) { return heap_name(run.heap); });
    if (!structure_section.empty()) {
        report += '\n' + structure_section;
    }
    return report;
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

std::string format_workload_table(const std::vector<WorkloadStats>& workloads, std::size_t operations, const WorkloadMix& mix) {
    std::ostringstream oss;
    oss << "=== Random PQ Workload Summary (" << operations << " ops) ===\n";
    oss << "Mix: Insert " << mix.insert_pct << "% | Decrease " << mix.decrease_pct
        << "% | Extract " << mix.extract_pct << "%\n";
    oss << std::left << std::setw(12) << "Heap" << std::right
        << std::setw(14) << "Runtime(ms)"
        << std::setw(14) << "Inserts"
        << std::setw(18) << "Insert Avg (us)"
        << std::setw(14) << "Extracts"
        << std::setw(18) << "Extract Avg (us)"
        << std::setw(14) << "Decreases"
        << std::setw(20) << "Decrease Avg (us)"
        << '\n';
    oss << std::string(128, '-') << '\n';
    oss << std::fixed << std::setprecision(3);
    for (const auto& run : workloads) {
        double insert_avg = average_us(run.metrics.insert_time_ns, run.metrics.insert_count);
        double extract_avg = average_us(run.metrics.extract_time_ns, run.metrics.extract_count);
        double decrease_avg = average_us(run.metrics.decrease_time_ns, run.metrics.decrease_count);
        oss << std::left << std::setw(12) << heap_name(run.heap) << std::right
            << std::setw(14) << run.total_runtime_ms
            << std::setw(14) << run.metrics.insert_count
            << std::setw(18) << insert_avg
            << std::setw(14) << run.metrics.extract_count
            << std::setw(18) << extract_avg
            << std::setw(14) << run.metrics.decrease_count
            << std::setw(20) << decrease_avg
            << '\n';
    }
    oss.unsetf(std::ios::floatfield);
    std::string report = oss.str();
    std::string structure_section = format_structure_table(
        workloads,
        "=== Structural Metrics for Random Workload (" + std::to_string(operations) + " ops) ===",
        [](const WorkloadStats& run) { return heap_name(run.heap); });
    if (!structure_section.empty()) {
        report += '\n' + structure_section;
    }
    return report;
}

template <typename HeapType, typename HandleType>
WorkloadStats run_workload_impl(std::size_t operations, std::uint32_t seed, const WorkloadMix& mix) {
    using Clock = std::chrono::steady_clock;
    std::mt19937 rng(seed);
    HeapType heap;
    QueueMetrics metrics;

    std::vector<HandleType*> handle_by_value;
    std::vector<long long> key_by_value;
    std::vector<int> active_ids;
    std::vector<int> active_pos;
    int next_value = 0;

    auto ensure_capacity = [&](int value) {
        if (value >= static_cast<int>(handle_by_value.size())) {
            handle_by_value.resize(value + 1, nullptr);
            key_by_value.resize(value + 1, 0);
            active_pos.resize(value + 1, -1);
        }
    };

    const int insert_threshold = mix.insert_pct;
    const int decrease_threshold = insert_threshold + mix.decrease_pct;
    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<long long> key_dist(1'000, 10'000'000);
    auto total_start = Clock::now();

    for (std::size_t i = 0; i < operations; ++i) {
        int choice = op_dist(rng);
        bool force_insert = heap.is_empty();
        bool can_decrease = !active_ids.empty();
        bool can_extract = !heap.is_empty() && (metrics.extract_count < metrics.insert_count);

        enum class PlannedOp { Insert, Decrease, Extract };
        PlannedOp planned = PlannedOp::Extract;
        if (choice < insert_threshold) {
            planned = PlannedOp::Insert;
        } else if (choice < decrease_threshold) {
            planned = PlannedOp::Decrease;
        }

        if (force_insert) {
            planned = PlannedOp::Insert;
        } else if (planned == PlannedOp::Decrease && !can_decrease) {
            planned = can_extract ? PlannedOp::Extract : PlannedOp::Insert;
        } else if (planned == PlannedOp::Extract && !can_extract) {
            planned = can_decrease ? PlannedOp::Decrease : PlannedOp::Insert;
        }

        if (planned == PlannedOp::Insert) {
            long long key = key_dist(rng);
            int value = next_value++;
            ensure_capacity(value);
            auto op_start = Clock::now();
            HandleType* handle = heap.insert(key, value);
            auto op_end = Clock::now();
            metrics.insert_count++;
            metrics.insert_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(op_end - op_start).count();

            handle_by_value[value] = handle;
            key_by_value[value] = key;
            active_pos[value] = static_cast<int>(active_ids.size());
            active_ids.push_back(value);
        } else if (planned == PlannedOp::Decrease) {
            std::uniform_int_distribution<std::size_t> idx_dist(0, active_ids.size() - 1);
            int value = active_ids[idx_dist(rng)];
            HandleType* handle = handle_by_value[value];
            if (!handle) {
                continue;
            }
            long long delta = 1 + static_cast<long long>(rng() % 1000);
            long long new_key = key_by_value[value];
            new_key = new_key > delta ? new_key - delta : 0;
            auto op_start = Clock::now();
            heap.decrease_key(handle, new_key);
            auto op_end = Clock::now();
            metrics.decrease_count++;
            metrics.decrease_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(op_end - op_start).count();
            key_by_value[value] = new_key;
        } else { // Extract
            auto op_start = Clock::now();
            auto result = heap.extract_min();
            auto op_end = Clock::now();
            metrics.extract_count++;
            metrics.extract_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(op_end - op_start).count();

            int value = result.second;
            if (value >= 0 && value < static_cast<int>(handle_by_value.size())) {
                handle_by_value[value] = nullptr;
                if (value < static_cast<int>(active_pos.size())) {
                    int pos = active_pos[value];
                    if (pos >= 0) {
                        int last_value = active_ids.back();
                        active_ids[pos] = last_value;
                        active_pos[last_value] = pos;
                        active_ids.pop_back();
                    }
                    active_pos[value] = -1;
                }
            }
        }
    }

    auto total_end = Clock::now();

    WorkloadStats stats;
    stats.operations = operations;
    stats.metrics = metrics;
    stats.total_runtime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();
    stats.structure = heap.structure_stats();
    return stats;
}

WorkloadStats run_random_workload(std::size_t operations, HeapSelection selection, std::uint32_t seed, const WorkloadMix& mix) {
    switch (selection) {
        case HeapSelection::kBinary: {
            auto stats = run_workload_impl<BinaryHeap, BinaryHeapNode>(operations, seed, mix);
            stats.heap = selection;
            return stats;
        }
        case HeapSelection::kFibonacci: {
            auto stats = run_workload_impl<FibonacciHeap, FibonacciHeapNode>(operations, seed, mix);
            stats.heap = selection;
            return stats;
        }
        case HeapSelection::kHollow: {
            auto stats = run_workload_impl<HollowHeap, HollowHeapNode>(operations, seed, mix);
            stats.heap = selection;
            return stats;
        }
        default:
            throw std::invalid_argument("Unknown heap selection");
    }
}
} // namespace

int main(int argc, char** argv) try {
    const std::filesystem::path exe_dir = detect_exe_directory(argv);
    const std::vector<DatasetOption> datasets = {
        {"Chongqing road network", "Data/Chongqing.road-d"},
        {"Hong Kong road network", "Data/Hongkong.road-d"},
        {"Shanghai road network", "Data/Shanghai.road-d"},
    };

    print_section_header("Dataset Selection");
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
    std::cout << "\nLoading " << dataset.name << "..." << std::endl;
    const auto dataset_path = resolve_dataset_path(dataset.path, exe_dir);
    if (dataset_path.empty()) {
        std::cerr << "Failed to locate dataset file: " << dataset.path << std::endl;
        return 1;
    }
    if (!graph.load_from_file(dataset_path.string(), &load_error)) {
        std::cerr << "Failed to load dataset: " << load_error << std::endl;
        return 1;
    }

    print_subsection_header("Graph Loaded");
    std::cout << "Dataset   : " << dataset.name << "\n";
    std::cout << "File      : " << dataset_path << "\n";
    std::cout << "Nodes     : " << graph.node_count() << "\n";
    std::cout << "Edges     : " << graph.edge_count() << std::endl;

    int source = read_int_with_default("Enter source vertex id [default: 0]: ", 0);
    if (source < 0 || static_cast<std::size_t>(source) >= graph.node_count()) {
        std::cout << "Source out of range. Using 0." << std::endl;
        source = 0;
    }

    print_section_header("Run Mode Selection");
    std::cout << "Select run mode:" << std::endl;
    std::cout << "  [1] Single run (interactive)" << std::endl;
    std::cout << "  [2] Run all heaps and produce summary" << std::endl;
    std::cout << "  [3] Random PQ workload benchmark" << std::endl;
    std::cout << "  [4] Run Dijkstra from every node" << std::endl;
    int mode_choice = read_int_with_default("Mode [default: 1]: ", 1);

    if (mode_choice == 2) {
        print_section_header("Batch Comparison (All Heaps)");
        std::vector<RunSummary> summaries;
        for (HeapSelection selection : {HeapSelection::kBinary, HeapSelection::kFibonacci, HeapSelection::kHollow}) {
            std::cout << "  • Running " << heap_name(selection) << " heap..." << std::flush;
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

    if (mode_choice == 3) {
        print_section_header("Random Priority-Queue Workload");
        int requested_ops = read_int_with_default("Total operations [default: 100000, max: 200000]: ", 100000);
        if (requested_ops < 1) {
            requested_ops = 1;
        }
        if (requested_ops > 200000) {
            std::cout << "Requested operations exceed limit; capping at 200000." << std::endl;
            requested_ops = 200000;
        }
        std::size_t op_count = static_cast<std::size_t>(requested_ops);

        WorkloadMix mix;
        mix.insert_pct = read_int_with_default("Insert percentage [default: 40]: ", 40);
        mix.decrease_pct = read_int_with_default("Decrease-key percentage [default: 35]: ", 35);
        mix.extract_pct = read_int_with_default("Extract-min percentage [default: 25]: ", 25);
        if (!mix.valid()) {
            std::cout << "Invalid mix; falling back to 40/35/25." << std::endl;
            mix = WorkloadMix{};
        }

        bool run_all = prompt_yes_no("Benchmark all heaps? [Y/n]: ", true);

        std::vector<WorkloadStats> workloads;
        std::random_device rd;
        const std::uint32_t seed = rd();

        auto run_for_selection = [&](HeapSelection selection) {
            std::cout << "  • Running " << heap_name(selection) << " workload..." << std::flush;
            WorkloadStats stats = run_random_workload(op_count, selection, seed, mix);
            std::cout << " done (" << stats.total_runtime_ms << " ms)." << std::endl;
            workloads.push_back(stats);
        };

        print_subsection_header("Configuration");
        std::cout << "Operations : " << op_count << "\n";
        std::cout << "Mix        : insert " << mix.insert_pct << "%, decrease "
              << mix.decrease_pct << "%, extract " << mix.extract_pct << "%" << std::endl;

        if (run_all) {
            for (HeapSelection selection : {HeapSelection::kBinary, HeapSelection::kFibonacci, HeapSelection::kHollow}) {
                run_for_selection(selection);
            }
        } else {
            std::cout << "Select heap implementation:" << std::endl;
            std::cout << "  [1] Binary Heap" << std::endl;
            std::cout << "  [2] Fibonacci Heap" << std::endl;
            std::cout << "  [3] Hollow Heap" << std::endl;
            int heap_choice = read_int_with_default("Choice [default: 3]: ", 3);
            if (heap_choice < 1 || heap_choice > 3) {
                std::cout << "Invalid selection. Using Hollow Heap." << std::endl;
                heap_choice = 3;
            }
            run_for_selection(static_cast<HeapSelection>(heap_choice));
        }

        auto default_path = default_workload_path(op_count);
        std::string out_path_input = read_line_with_default(
            "Enter workload summary file path [default: " + default_path.string() + "]: ",
            default_path.string());
        std::filesystem::path out_path(out_path_input);

        std::string report = format_workload_table(workloads, op_count, mix);
        std::cout << '\n' << report << std::endl;
        write_summary_report(report, out_path);
        std::cout << "Summary written to " << out_path << std::endl;
        return 0;
    }

    if (mode_choice == 4) {
        print_section_header("All-Sources Sweep");
        std::size_t total_nodes = graph.node_count();
        if (total_nodes == 0) {
            std::cout << "Graph has no nodes to process." << std::endl;
            return 0;
        }

        int start_input = read_int_with_default("Start source vertex id [default: 0]: ", 0);
        if (start_input < 0) {
            start_input = 0;
        }
        if (static_cast<std::size_t>(start_input) >= total_nodes) {
            std::cout << "Start vertex exceeds graph size. Using last vertex instead." << std::endl;
            start_input = static_cast<int>(total_nodes - 1);
        }
        std::size_t start_source = static_cast<std::size_t>(start_input);
        std::size_t remaining = total_nodes - start_source;

        int limit_input = read_int_with_default(
            "How many sources to process? [0 = all remaining]: ", 0);
        if (limit_input < 0) {
            limit_input = 0;
        }
        std::size_t sources_to_run = remaining;
        if (limit_input > 0) {
            sources_to_run = std::min(remaining, static_cast<std::size_t>(limit_input));
        }

        if (sources_to_run == 0) {
            std::cout << "No sources available to process from the chosen start vertex." << std::endl;
            return 0;
        }

        bool run_all_heaps = prompt_yes_no("Run all heap implementations? [Y/n]: ", true);
        std::cout << "Running Dijkstra from " << sources_to_run << " sources per heap (starting at vertex "
              << start_source << "). This may take a while." << std::endl;

        std::vector<AggregateStats> aggregates;
        auto run_for_selection = [&](HeapSelection selection) {
            print_subsection_header(heap_name(selection) + std::string(" Heap"));
            std::cout << "Beginning all-sources pass..." << std::endl;
            AggregateStats agg;
            agg.heap = selection;
            std::size_t progress_step = std::max<std::size_t>(1, sources_to_run / 10);
            for (std::size_t offset = 0; offset < sources_to_run; ++offset) {
                int source_vertex = static_cast<int>(start_source + offset);
                RunSummary summary = execute_run(graph, source_vertex, selection, nullptr);
                accumulate_aggregate(agg, summary);
                if ((offset + 1) % progress_step == 0 || offset + 1 == sources_to_run) {
                    std::cout << "  • Completed " << (offset + 1) << "/" << sources_to_run
                              << " sources\r" << std::flush;
                }
            }
            std::cout << std::string(50, ' ') << "\r";
            std::cout << "  Completed " << sources_to_run << " sources for " << heap_name(selection) << "."
                      << std::endl;
            aggregates.push_back(agg);
        };

        if (run_all_heaps) {
            for (HeapSelection selection : {HeapSelection::kBinary, HeapSelection::kFibonacci, HeapSelection::kHollow}) {
                run_for_selection(selection);
            }
        } else {
            std::cout << "Select heap implementation:" << std::endl;
            std::cout << "  [1] Binary Heap" << std::endl;
            std::cout << "  [2] Fibonacci Heap" << std::endl;
            std::cout << "  [3] Hollow Heap" << std::endl;
            int heap_choice = read_int_with_default("Choice [default: 3]: ", 3);
            if (heap_choice < 1 || heap_choice > 3) {
                std::cout << "Invalid selection. Using Hollow Heap." << std::endl;
                heap_choice = 3;
            }
            run_for_selection(static_cast<HeapSelection>(heap_choice));
        }

        auto default_path = default_all_sources_path(dataset, start_source, sources_to_run);
        std::string out_path_input = read_line_with_default(
            "Enter all-sources summary file path [default: " + default_path.string() + "]: ",
            default_path.string());
        std::filesystem::path out_path(out_path_input);

        std::string report = format_all_sources_table(aggregates, dataset.name, sources_to_run);
        std::string structure_section = format_structure_table(
            aggregates,
            "=== Structural Peaks for " + dataset.name + " (all-sources) ===",
            [](const AggregateStats& agg) { return heap_name(agg.heap); });
        if (!structure_section.empty()) {
            report += '\n' + structure_section;
        }

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

    print_section_header("Run Summary");
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

    std::cout << "(Dijkstra already processed the full dataset; the next prompt only controls how many results to display.)" << std::endl;

    print_subsection_header("Structural Metrics");
    print_structure_metrics(summary.structure);

    std::cout.unsetf(std::ios::floatfield);
    std::cout << std::setprecision(6);

    int sample_limit = read_int_with_default(
        "How many reachable nodes to display? [0 = all, default: 10]: ", 10);
    if (sample_limit < 0) {
        sample_limit = 0;
    }

    if (sample_limit == 0) {
        std::cout << "\nDistances for all reachable nodes:" << std::endl;
    } else {
        std::cout << "\nSample distances (first " << sample_limit << " reachable nodes):" << std::endl;
    }
    print_distance_sample(result.distances, sample_limit);

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}
catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
}
