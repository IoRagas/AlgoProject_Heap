#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <utility>
#include <functional>
#include <iomanip>
#include "BinaryHeap.h"

using namespace std;

int main() {
    BinaryHeap pq;
    // CHECKING THE DECREASE KEY
    auto node1 = pq.insert(10, 1);
    auto node2 = pq.insert(20, 2);
    pq.decrease_key(node2, 5); // Decrease key of node2 to 5
    auto min1 = pq.extract_min(); // Should be (5, 2)
    cout << "Extracted min after decrease_key: (" << min1.first << ", " << min1.second << ")\n";
    auto min2 = pq.extract_min(); // Should be (10, 1)
    cout << "Extracted min: (" << min2.first << ", " << min2.second << ")\n";
    cout << pq.is_empty() << endl;

    cin.get(); //To keep the console window open
    return 0;
}
