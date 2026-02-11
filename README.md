![static_radix_map banner](assets/banner.png)

# static_radix_map

`static_radix_map` is a highly efficient C++ template class for **static mapping**. It is designed for scenarios where the set of keys is known at compile-time or initialization-time, allowing for a near-optimal multiway tree (Radix Tree/Trie) structure.

## Overview

Unlike standard hash maps (`std::unordered_map`) or balanced binary trees (`std::map`), the `static_radix_map` analyzes the specific bit-patterns of all keys during construction to create a custom jump-table based search tree.

### Key Features
*   **Near-Optimal Search**: Uses a greedy discrimination algorithm to select the most selective bytes for branching.
*   **Zero Hashing**: No hash functions are computed during lookup, eliminating hash collision overhead.
*   **Static Context**: Ideal for fixed lookups like command parsers, routing tables, or configuration mappings.
*   **STL-like Interface**: Implements a subset of `std::map` (find, count, iterators).
*   **Variable Length Type Support**: Native specializations for `std::string` and `const char*`.

## Project Structure

```text
static_radix_map/
├── assets/             # Project graphics
├── benchmark/          # Performance tests
├── include/            # Header-only library
│   └── static_radix_map/
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

## Evaluation & Benchmarks

### Performance Comparison (M1 Max)
Benchmarks show that `static_radix_map` consistently outperforms `std::unordered_map` for small to medium-sized datasets.

**Benchmark Environment:**
*   **CPU**: Apple M1 Max (arm64)
*   **Compiler**: Apple clang version 17.0.0 (`-O3`)
*   **C++ Version**: C++11 (`-std=c++11`)
*   **Libraries**: Boost 1.87

| Key Count | `std::unordered_map` (sec) | `static_radix_map` (sec) | Speedup |
| :--- | :--- | :--- | :--- |
| 16 | 0.241 | **0.159** | +34% |
| 64 | 0.237 | **0.180** | +24% |
| 256 | 0.245 | **0.179** | +27% |
| 1024 | 0.239 | **0.225** | +6% |
| 5000 | 0.318 | **0.236** | +26% |
| 10000 | 0.320 | **0.306** | +4% |

*Note: Benchmarks performed with 10,000,000 lookups per test.*

### Analysis

#### Strengths
*   **Extremely Low Latency**: Faster than hashing for small $N$ because it avoids the computational cost of the hash function.
*   **Deterministic Lookup**: The search time is bounded by the number of discriminator bytes, not by bucket collisions.
*   **Memory Efficiency for Small Sets**: Very compact for a handful of keys.

#### Limitations
*   **Construction Overhead**: The initialization is $O(N \cdot L)$, making it unsuitable for frequent updates.
*   **Cache Locality**: On very large datasets (> 10k keys), the performance advantage diminishes. This is due to "pointer chasing" across individually allocated nodes, which can lead to CPU cache misses.
*   **Sparse Byte Distribution**: If keys have very sparse discriminator bytes, the dispatch tables (up to 256 slots) might consume more memory than a compact hash table.

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
