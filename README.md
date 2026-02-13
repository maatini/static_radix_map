![static_radix_map banner](assets/banner.png)

# static_radix_map

`static_radix_map` is a highly efficient C++ template class for **static mapping**. It is designed for scenarios where the set of keys is known at compile-time or initialization-time, allowing for a near-optimal multiway tree (Radix Tree/Trie) structure.

## Overview

Unlike standard hash maps (`std::unordered_map`) or balanced binary trees (`std::map`), the `static_radix_map` analyzes the specific bit-patterns of all keys during construction to create a custom jump-table based search tree.

### Key Features
*   **Zero Dependencies**: Header-only, C++17 standard library only — no Boost or other external libraries required.
*   **Contiguous Memory Layout**: The entire tree structure is flattened into a single contiguous buffer (vector of uint32_t), eliminating memory fragmentation and improving cache locality.
*   **Near-Optimal Search**: Uses a greedy discrimination algorithm to select the most selective bytes for branching.
*   **Zero Hashing**: No hash functions are computed during lookup, eliminating hash collision overhead.
*   **Deterministic Lookup**: Search time is bounded by the number of discriminator bytes, with constant-time jump-table lookups.
*   **STL-like Interface**: Implements a subset of `std::map` (find, count, iterators, equal_range).
*   **Variable Length Type Support**: Native specializations for `std::string` and `const char*`.
*   **Move Semantics**: Full move support for efficient transfers.

## Project Structure

```text
static_radix_map/
├── assets/             # Project graphics
├── benchmark/          # Performance tests
├── include/            # Header-only library
├── tests/              # Unit tests (18 test cases)
├── CMakeLists.txt      # Build system (C++17)
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

1.  **Preprocessing**: During construction, the algorithm identifies the "best" byte index (discriminator) that maximizes selectivity among the current subset of keys.
2.  **Multiway Branching**: For each node, a dispatch table is created based on the identified discriminator byte.
3.  **Greedy Optimization**: The tree height is minimized by picking indexes that split the key space as efficiently as possible.
4.  **Fallback Semantics**: Safely handles keys with common prefixes or different lengths.

### Tree Structure (Flattened)

The tree is serialized into a contiguous `std::vector<uint32_t>`, which makes lookups extremely fast and cache-friendly. The "pointer chasing" typically associated with tree structures is mitigated by the dense data layout.

## Evaluation & Benchmarks

### Performance Comparison (M1 Max)
`static_radix_map` consistently outperforms both `std::unordered_map` and `std::map`. Results are averaged over 10 runs of 10M lookups each.

| Key Count | `std::unordered_map` (sec) | `std::map` (sec) | `static_radix_map` (sec) | vs unordered | vs map |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 16 | 0.2435 | 0.4205 | **0.1553** | **+36%** | **+63%** |
| 64 | 0.2343 | 0.6338 | **0.1830** | **+22%** | **+71%** |
| 256 | 0.2371 | 0.8311 | **0.1769** | **+25%** | **+79%** |
| 1024 | 0.2376 | 1.1429 | **0.2054** | **+14%** | **+82%** |
| 5000 | 0.3138 | 1.8297 | **0.2139** | **+32%** | **+88%** |
| 10000 | 0.3199 | 2.0961 | **0.2573** | **+20%** | **+88%** |

**Overall**: `static_radix_map` won **6/6** size categories.

### Absent-Key Rejection Performance
A particular strength is the rejection speed for absent (non-existing) keys:

| Key Count | `std::unordered_map` (sec) | `std::map` (sec) | `static_radix_map` (sec) |
| :--- | :--- | :--- | :--- |
| 16 | 0.0218 | 0.0348 | **0.0033** |
| 256 | 0.0208 | 0.0993 | **0.0018** |
| 5000 | 0.0277 | 0.2018 | **0.0019** |

*Absent-key rejection is ~7–15× faster than `std::unordered_map`.*

### Integrity Verified
The implementation includes a robust tagging system for the contiguous memory buffer. All lookup checksums match across implementations, ensuring 100% correctness and deterministic results.

### Analysis

#### Strengths
*   **Zero Fragmentation**: The entire map consists of exactly two memory blocks after construction (one for data, one for the tree).
*   **Cache Locality**: Contiguous buffer layout minimizes cache misses during tree traversal.
*   **Extremely Low Latency**: Faster than hashing even for larger $N$ due to the elimination of hash computation.
*   **Deterministic Lookup**: The search time is strictly bounded.
*   **Outstanding Absent-Key Performance**: Non-existing keys are rejected ~10× faster than hash-based lookups.

#### Limitations
*   **Construction Overhead**: Initialization is $O(N \cdot L)$, making it unsuitable for frequent updates.
*   **Sparse Byte Distribution**: If keys have very sparse discriminator bytes, the dispatch tables might consume more memory than a compact hash table.

## Requirements & Usage

### Requirements
*   **C++17 compiler** (GCC ≥ 7, Clang ≥ 5, MSVC ≥ 19.14)
*   No external dependencies — standard library only.
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

    radix::static_radix_map<std::string, int> smap(data);

    int val = smap.value("apple"); // Very fast lookup
    return 0;
}
```

### Building & Testing
```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure

# Run benchmark
./map_perf

# Build with sanitizers
cmake .. -DENABLE_SANITIZERS=ON
cmake --build .
```

## License
Copyright (c) 2010 Martin Richardt. Distributed under the MIT License.
