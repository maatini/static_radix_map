![static_radix_map banner](assets/banner.png)

# static_radix_map

`static_radix_map` is a highly efficient C++ template class for **static mapping**. It is designed for scenarios where the set of keys is known at compile-time or initialization-time, allowing for a near-optimal multiway tree (Radix Tree/Trie) structure.

## Overview

Unlike standard hash maps (`std::unordered_map`) or balanced binary trees (`std::map`), the `static_radix_map` analyzes the specific bit-patterns of all keys during construction to create a custom jump-table based search tree.

### Key Features
*   **Contiguous Memory Layout**: The entire tree structure is flattened into a single contiguous buffer (vector of uint32_t), eliminating memory fragmentation and improving cache locality.
*   **Near-Optimal Search**: Uses a greedy discrimination algorithm to select the most selective bytes for branching.
*   **Zero Hashing**: No hash functions are computed during lookup, eliminating hash collision overhead.
*   **Deterministic Lookup**: Search time is bounded by the number of discriminator bytes, with constant-time jump-table lookups.
*   **STL-like Interface**: Implements a subset of `std::map` (find, count, iterators).
*   **Variable Length Type Support**: Native specializations for `std::string` and `const char*`.

## Project Structure

```text
static_radix_map/
├── assets/             # Project graphics
├── benchmark/          # Performance tests
├── include/            # Header-only library
├── tests/              # Unit tests
├── CMakeLists.txt      # Build system
├── README.md
└── LICENSE
```

## Algorithm Details

The implementation is based on a **Radix Tree** that is optimized for the provided set of keys.

### Lookup Process

The lookup is an iterative process that jumps through levels of the tree using specific key bytes as indexes.

![Lookup Flow](assets/lookup_flow.png)

### Tree Structure Example

For a set of keys like `{"apple", "ant", "banana"}`, the tree might look like this (simplified representation):

![Tree Structure](assets/tree_structure.png)

1.  **Preprocessing**: During construction, the algorithm identifies the "best" byte index (discriminator) that maximizes selectivity among the current subset of keys.
2.  **Multiway Branching**: For each node, a dispatch table is created based on the identified discriminator byte.
3.  **Greedy Optimization**: The tree height is minimized by picking indexes that split the key space as efficiently as possible.
4.  **Fallback Semantics**: Safely handles keys with common prefixes or different lengths.

### Tree Structure (Flattened)

The tree is serialized into a contiguous `std::vector<uint32_t>`, which makes lookups extremely fast and cache-friendly. The "pointer chasing" typically associated with tree structures is mitigated by the dense data layout.

## Evaluation & Benchmarks

### Performance Comparison (M1 Max)
After the contiguous buffer optimization, `static_radix_map` shows a significant lead over `std::unordered_map`.

| Key Count | `std::unordered_map` (sec) | `static_radix_map` (sec) | Speedup |
| :--- | :--- | :--- | :--- |
| 16 | 0.244 | **0.137** | **+44%** |
| 64 | 0.233 | **0.152** | **+35%** |
| 256 | 0.239 | **0.114** | **+52%** |
| 1024 | 0.246 | **0.101** | **+59%** |
| 5000 | 0.315 | **0.169** | **+46%** |
| 10000 | 0.320 | **0.168** | **+47%** |

*Note: Benchmarks performed with 10,000,000 lookups per test.*

### Analysis

#### Strengths
*   **Zero Fragmentation**: The entire map consists of exactly two memory blocks after construction (one for data, one for the tree).
*   **Cache Locality**: Contiguous buffer layout minimizes cache misses during tree traversal.
*   **Extremely Low Latency**: Faster than hashing even for larger $N$ due to the elimination of hash computation.
*   **Deterministic Lookup**: The search time is strictly bounded.

#### Limitations
*   **Construction Overhead**: Initialization is $O(N \cdot L)$, making it unsuitable for frequent updates.
*   **Sparse Byte Distribution**: If keys have very sparse discriminator bytes, the dispatch tables might consume more memory than a compact hash table.

## Requirements & Usage

### Requirements
*   **Boost Libraries**: Depends on Boost (shared_ptr, tuple, mpl, type_traits). Tested with Boost >= 1.36.
*   **Key Property**: Keys must have the representational equality property (memcmp-safe).

### Integration via CMake
```cmake
# In your CMakeLists.txt
add_subdirectory(path/to/static_radix_map)
target_link_libraries(your_target PRIVATE static_radix_map)
```

### Usage Example
```cpp
#include <static_radix_map/static_radix_map.hpp>
#include <string>
#include <map>

int main() {
    std::map<std::string, int> data;
    data["apple"] = 1;
    data["banana"] = 2;

    static_map_stuff::static_radix_map<std::string, int> smap(data);

    int val = smap.value("apple"); // Very fast lookup
    return 0;
}
```

## License
Copyright (c) 2010 Martin Richardt. Distributed under the MIT License.
