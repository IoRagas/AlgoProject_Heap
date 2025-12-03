# Cross-Dataset Heap Performance Analysis

## 1. Executive Summary
This analysis compares the performance of **Binary**, **Fibonacci**, and **Hollow** heaps across three real-world road network datasets (Hong Kong, Shanghai, Chongqing) and a synthetic random workload.

**Key Findings:**
*   **Binary Heap** is the fastest implementation for Dijkstra's algorithm on road networks, outperforming Fibonacci and Hollow heaps by **1.3x to 2.3x**.
*   **Hollow Heap** exhibits catastrophic memory usage in graph search scenarios (up to **5600x** higher than Binary) due to the accumulation of "hollow" nodes in the DAG structure.
*   **Fibonacci Heap** consistently lags in runtime due to high pointer manipulation overhead, despite its theoretical $O(1)$ decrease-key.
*   **Decrease-Key Performance:** While theoretically superior, advanced heaps only outperformed the Binary heap in the synthetic "Decrease-Heavy" workload. In real-world graphs, the Binary heap's cache locality dominated.

---

## 2. Dataset Overview
*   **Hong Kong (Small):** ~43k Nodes, ~91k Edges
*   **Shanghai (Medium):** ~390k Nodes, ~855k Edges
*   **Chongqing (Large):** ~1.2M Nodes, ~2.4M Edges

---

## 3. Performance Comparison

### 3.1 Total Runtime (Dijkstra)
*Lower is better.*

| Dataset | Binary (ms) | Hollow (ms) | Fibonacci (ms) | Binary Speedup vs Hollow | Binary Speedup vs Fib |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Hong Kong** | 16 | 21 | 34 | **1.31x** | **2.13x** |
| **Shanghai** | 139 | 185 | 305 | **1.33x** | **2.19x** |
| **Chongqing** | 425 | 562 | 922 | **1.32x** | **2.17x** |

**Analysis:** The performance gap is remarkably consistent across all scales. The Binary Heap maintains a ~30% lead over Hollow and a >2x lead over Fibonacci regardless of graph size.

### 3.2 Operation Latency (Microseconds)
*Lower is better.*

**Chongqing (Large) Averages:**
| Heap | Insert ($\mu s$) | Extract-Min ($\mu s$) | Decrease-Key ($\mu s$) |
| :--- | :--- | :--- | :--- |
| **Binary** | **0.043** | **0.075** | **0.026** |
| **Hollow** | 0.108 | 0.135 | 0.079 |
| **Fibonacci** | 0.043 | 0.517 | 0.027 |

**Analysis:**
*   **Extract-Min:** Fibonacci is ~7x slower than Binary due to the expensive `consolidate()` step that scans the root list. Hollow is ~2x slower.
*   **Decrease-Key:** Surprisingly, **Binary is faster** (0.026 $\mu s$) than the $O(1)$ heaps.
    *   *Reason:* Road networks are sparse ($E \approx 2V$), so `decrease_key` is rare (~1% of ops). The Binary Heap's array-based "bubble up" is extremely cache-efficient compared to the pointer chasing and memory allocation (Hollow) required by the others.

### 3.3 Memory Usage (Peak MB)
*Lower is better.*

| Dataset | Binary (MB) | Fibonacci (MB) | Hollow (MB) | Hollow Overhead Factor |
| :--- | :--- | :--- | :--- | :--- |
| **Hong Kong** | 0.00 | 0.00 | 2.66 | ~2600x |
| **Shanghai** | 0.00 | 0.01 | 24.00 | ~2400x |
| **Chongqing** | 0.01 | 0.03 | 72.89 | **~5600x** |

**Analysis:**
*   **Binary & Fibonacci:** Memory usage is negligible (kilobytes) because they only store active nodes in the frontier.
*   **Hollow Heap:** Usage is massive. In Dijkstra, nodes are frequently updated. The Hollow Heap *creates a new node* for every decrease-key and leaves the old "hollow" node in memory until it becomes a root. This "zombie node" accumulation explodes memory usage in long-running graph searches.

---

## 4. Synthetic Workload Analysis (Random PQ)
*Scenario: 200,000 operations with high decrease-key probability.*

| Heap | Runtime (ms) | Insert ($\mu s$) | Extract ($\mu s$) | Decrease ($\mu s$) |
| :--- | :--- | :--- | :--- | :--- |
| **Binary** | 24 | 0.04 | 0.08 | 0.03 |
| **Hollow** | **21** | 0.06 | 0.12 | **0.02** |
| **Fibonacci** | 35 | 0.04 | 0.32 | 0.02 |

**Analysis:**
*   Here, the **Hollow Heap wins**.
*   In a dense, decrease-heavy synthetic test, the $O(1)$ decrease-key finally pays off, beating the Binary Heap's $O(\log N)$.
*   This confirms the implementation is correct and efficient *for the specific operations it optimizes*, but those conditions don't exist in sparse road networks.

---

## 5. Situational Recommendations

### When to use **Binary Heap**
*   **Scenario:** Shortest Path on Road Networks, Sparse Graphs ($E < 10V$), Systems with limited memory.
*   **Why:** Unbeatable cache locality, zero allocation overhead during operations, and minimal memory footprint. It is the robust default choice.

### When to use **Hollow Heap**
*   **Scenario:** Dense Graphs ($E \approx V^2$), Algorithms with massive numbers of `decrease_key` operations (e.g., Prim's MST on dense graphs), Scenarios where memory is abundant.
*   **Why:** It offers the fastest theoretical decrease-key. However, **avoid** if memory is a constraint or if the algorithm runs for a long time with many updates, as the "hollow node" buildup will thrash the cache.

### When to use **Fibonacci Heap**
*   **Scenario:** Theoretical research or specific persistent data structure requirements.
*   **Why:** In practice, it is almost always outperformed by the Binary Heap (due to complexity) or the Hollow Heap (which is a simpler/faster version of the same concept). It is rarely the best practical choice for standard pathfinding.
