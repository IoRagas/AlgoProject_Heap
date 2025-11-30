/*
#include <iostream>
#include <vector>
#include <algorithm>
#include <vector>
#include "BinaryHeap.h"
#include "FibonacciHeap.h"
#include "HollowHeap.h"

using namespace std;
int main() {
    constexpr int kNodeCount = 250;
    HollowHeap pq;
    vector<HollowHeapNode*> handles(kNodeCount, nullptr);
    vector<long long> finalKeys(kNodeCount, 0);
    vector<int> values(kNodeCount, 0);
    
    cout << "=== Bulk Insert Phase (" << kNodeCount << " nodes) ===\n";
    for (int i = 0; i < kNodeCount; ++i) {
        long long baseKey = 1'000'000 + static_cast<long long>(i) * 1'000;
        int value = 1000 + i;
        handles[i] = pq.insert(baseKey, value);
        finalKeys[i] = baseKey;
        values[i] = value;
    }
    
    cout << "\n=== Decrease-Key Phase ===\n";
    auto apply_decrease = [&](int idx, long long delta) {
        long long newKey = finalKeys[idx] - delta;
        pq.decrease_key(handles[idx], newKey);
        finalKeys[idx] = newKey;
        cout << "Node #" << idx << " (value " << values[idx] << ") -> new key " << newKey << '\n';
    };
    
    for (int i = 0; i < kNodeCount; i += 3) {
        apply_decrease(i, 200 + (i % 17));
    }
    for (int i = 1; i < kNodeCount; i += 5) {
        apply_decrease(i, 120 + (i % 19));
    }
    for (int i = 2; i < kNodeCount; i += 11) {
        apply_decrease(i, 80 + (i % 13));
    }
    
    vector<pair<long long, int>> expected;
    expected.reserve(kNodeCount);
    for (int i = 0; i < kNodeCount; ++i) {
        expected.emplace_back(finalKeys[i], values[i]);
    }
    sort(expected.begin(), expected.end());
    
    cout << "\n=== Extract-Min Verification ===\n";
    for (int i = 0; i < kNodeCount; ++i) {
        auto result = pq.extract_min();
        const auto& exp = expected[i];
        bool match = (result == exp);
        cout << "Extract #" << (i + 1) << ": (" << result.first << ", " << result.second
        << ")  // Expected (" << exp.first << ", " << exp.second << ") -> "
        << (match ? "OK" : "MISMATCH") << '\n';
    }
    
    cout << "\nQueue empty? " << boolalpha << pq.is_empty() << "  // Expected: true\n";
    
    cin.get();
    return 0;
}
*/
