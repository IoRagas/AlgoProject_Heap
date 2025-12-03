# Cross-Dataset Heap Performance Analysis

## 1. Executive Summary
This report analyzes the performance of **Binary Heap**, **Fibonacci Heap**, and **Hollow Heap** across three real-world road network datasets (Hong Kong, Shanghai, Chongqing) and two synthetic random workloads. 

**Key Findings:**
*   **Binary Heap** is the consistent winner for Dijkstra's algorithm on road networks, outperforming Fibonacci and Hollow heaps by **1.3x to 2.3x**.
*   **Hollow Heap** demonstrates superior performance in **decrease-key heavy workloads** (synthetic), beating Binary Heap by ~15%, but suffers from **massive memory overhead** (up to 5000x) in graph applications.
*   **Fibonacci Heap** consistently trails behind due to high constant factors, despite its theoretical asymptotic advantages.

---

## 2. Dataset Analysis

### 2.1. Road Network: Hong Kong (Small)
*   **Nodes/Edges:** Small scale test.
*   **Winner:** Binary Heap (15ms).
*   **Comparison:**
    *   Binary is **1.93x faster** than Fibonacci.
    *   Binary is **1.27x faster** than Hollow.

### 2.2. Road Network: Shanghai (Medium)
*   **Winner:** Binary Heap (141ms).
*   **Comparison:**
    *   Binary is **2.33x faster** than Fibonacci.
    *   Binary is **1.47x faster** than Hollow.

### 2.3. Road Network: Chongqing (Large)
*   **Winner:** Binary Heap (425ms).
*   **Comparison:**
    *   Binary is **2.17x faster** than Fibonacci.
    *   Binary is **1.32x faster** than Hollow.

### 2.4. Synthetic Workloads
*   **Decrease-Heavy (80% Decrease-Key):** **Hollow Heap** wins (27ms vs Binary 32ms). This confirms the theoretical advantage of $O(1)$ decrease-key operations in specific dense-update scenarios.
*   **Insert-Heavy (90% Insert):** **Binary Heap** wins (65ms), closely followed by Hollow (69ms) and Fibonacci (71ms).

---

## 3. Detailed Metrics

### 3.1. Runtime Comparison (ms)

| Dataset | Binary | Fibonacci | Hollow |
| :--- | :--- | :--- | :--- |
| **Hong Kong** | **15** | 29 | 19 |
| **Shanghai** | **141** | 329 | 207 |
| **Chongqing** | **425** | 922 | 562 |
| **Random (Decr)**| 32 | 45 | **27** |
| **Random (Ins)** | **65** | 71 | 69 |

### 3.2. Operation Averages (microseconds)
*Data from Chongqing (Large Graph)*

| Operation | Binary | Fibonacci | Hollow | Note |
| :--- | :--- | :--- | :--- | :--- |
| **Insert** | **0.043** | 0.043 | 0.108 | Binary/Fib tied for fastest insertion. |
| **Extract-Min** | **0.075** | 0.517 | 0.135 | Binary is 7x faster than Fib, 1.8x faster than Hollow. |
| **Decrease-Key**| **0.026** | 0.027 | 0.079 | Binary matches Fib in practice; Hollow lags due to overhead. |

### 3.3. Memory Usage (Max MB)
*Data from Chongqing (Large Graph)*

| Heap | Max Memory (MB) | Overhead Factor (vs Binary) |
| :--- | :--- | :--- |
| **Binary** | 0.013 | 1x |
| **Fibonacci**| 0.030 | 2.3x |
| **Hollow** | **72.886** | **5606x** |

**Observation:** Hollow Heap's memory usage in graph algorithms is catastrophic. This is likely due to the "hollow" nodes (stale nodes left in the structure after decrease-key) accumulating in the large "fringe" of Dijkstra's algorithm, which are not cleaned up until extraction.

### 3.4. Structural Metrics
*Data from Chongqing (Large Graph)*

| Heap | Max Nodes | Link Ops | Consolidation Passes |
| :--- | :--- | :--- | :--- |
| **Binary** | 499 | 7,308,290 | 1,185,372 |
| **Fibonacci**| 499 | 6,689,216 | 1,185,449 |
| **Hollow** | 498 | 11,657,073 | 1,185,464 |

**Note:** Hollow Heap performs significantly more "Link Ops" (11.6M vs 7.3M), which contributes to its higher runtime despite the theoretical efficiency.

---

## 4. Comparative Analysis

### 4.1. Speedup Factors
Binary Heap consistently provides a **2x speedup** over Fibonacci Heap in pathfinding tasks. The gap remains stable as the dataset size increases (Hong Kong -> Chongqing), suggesting that for sparse graphs like road networks, the $O(\log N)$ overhead of Binary Heap is negligible compared to the constant factor overhead of pointer-based heaps.

### 4.2. Decrease-Key Efficiency
Theoretically, Fibonacci and Hollow heaps have $O(1)$ decrease-key. However, in practice:
*   **Graph Context:** Binary Heap's decrease-key (0.026us) was just as fast as Fibonacci (0.027us). This is because the heap size in Dijkstra is often much smaller than $V$ (only the frontier nodes), keeping $\log N$ small.
*   **Synthetic Context:** In the 80% decrease-key workload, Hollow Heap finally shined, outperforming Binary by ~15%. This proves the theoretical benefit exists but requires a very specific, update-intensive workload to overcome the implementation overhead.

### 4.3. Memory Overhead
Hollow Heap is not memory-safe for large graph traversals in its current form. Using 72MB vs 0.013MB for the same task is a disqualifying factor for memory-constrained environments. Binary Heap is extremely memory efficient, storing data in a flat array.

---

## 5. Situational Recommendations

Based on the analysis, here are the recommendations for choosing a heap:

### **Use Binary Heap When:**
*   **General Purpose:** It is the best default choice for almost all standard applications.
*   **Graph Algorithms:** For Dijkstra or Prim on sparse graphs (like road networks), it is unbeatable in speed and memory efficiency.
*   **Memory Constraints:** It has the lowest memory footprint by far.

### **Use Hollow Heap When:**
*   **Dense Updates:** The workload is dominated by `decrease-key` operations (>50% of ops) AND memory is abundant.
*   **Theoretical Research:** You need to demonstrate amortized bounds or are working on algorithms where $O(1)$ decrease-key is strictly required for complexity proofs.

### **Use Fibonacci Heap When:**
*   **Rarely:** In practice, Fibonacci heaps are often too slow due to pointer overhead and cache misses. Hollow Heap is generally a better alternative if $O(1)$ decrease-key is needed, provided memory is not an issue.
