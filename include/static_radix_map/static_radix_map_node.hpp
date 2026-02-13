//----------------------------------------------------------------------------
//
// Copyright (c) 2010 Martin Richardt
//
//      email: martin.richardt@web.de
//
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies, substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//----------------------------------------------------------------------------

#ifndef STATIC_MAP_RADIX_NODE_HPP

#define STATIC_MAP_RADIX_NODE_HPP

#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace radix {

namespace detail {

// Map data abstraction.
// Key must have the representational equality property

template <class Key, class Mapped>
struct MapDataT : public std::pair<Key, Mapped> {
  MapDataT(const Key &key, const Mapped &value)
      : std::pair<Key, Mapped>(key, value) {}

  MapDataT(const std::pair<Key, Mapped> &p) : std::pair<Key, Mapped>(p) {}

  std::size_t size() const noexcept { return sizeof(Key); }

  operator const char *() const noexcept {
    return reinterpret_cast<const char *>(&key());
  }

  const Key &key() const noexcept { return std::pair<Key, Mapped>::first; }

  Mapped &value() noexcept { return std::pair<Key, Mapped>::second; }

  const Mapped &value() const noexcept {
    return std::pair<Key, Mapped>::second;
  }
};

// specialization for std::string
template <class Mapped>
struct MapDataT<std::string, Mapped> : public std::pair<std::string, Mapped> {
  MapDataT(const std::string &key, const Mapped &value)
      : std::pair<std::string, Mapped>(key, value) {}

  MapDataT(const std::pair<std::string, Mapped> &p)
      : std::pair<std::string, Mapped>(p) {}

  std::size_t size() const noexcept {
    return std::pair<std::string, Mapped>::first.size();
  }

  operator const char *() const noexcept { return key().c_str(); }

  const std::string &key() const noexcept {
    return std::pair<std::string, Mapped>::first;
  }

  Mapped &value() noexcept { return std::pair<std::string, Mapped>::second; }

  const Mapped &value() const noexcept {
    return std::pair<std::string, Mapped>::second;
  }
};

// specialization for const char*
template <class Mapped>
struct MapDataT<const char *, Mapped> : public std::pair<const char *, Mapped> {
  std::size_t size_;

  MapDataT(const char *key, const Mapped &value)
      : std::pair<const char *, Mapped>(key, value), size_(std::strlen(key)) {}

  MapDataT(const std::pair<const char *, Mapped> &p)
      : std::pair<const char *, Mapped>(p), size_(std::strlen(p.first)) {}

  std::size_t size() const noexcept { return size_; }

  operator const char *() const noexcept {
    return std::pair<const char *, Mapped>::first;
  }

  const char *key() const noexcept {
    return std::pair<const char *, Mapped>::first;
  }

  Mapped &value() noexcept { return std::pair<const char *, Mapped>::second; }

  const Mapped &value() const noexcept {
    return std::pair<const char *, Mapped>::second;
  }
};

// ----------------------------
// Helper functions for key byte access (all inline to avoid ODR violations)

inline const char *to_const_char(std::string &s) noexcept { return s.c_str(); }

inline const char *to_const_char(const std::string &s) noexcept {
  return s.c_str();
}

inline const char *to_const_char(const char *s) noexcept { return s; }

template <typename T> inline const char *to_const_char(const T &x) noexcept {
  return reinterpret_cast<const char *>(&x);
}

inline std::size_t to_size(const std::string &s) noexcept { return s.size(); }

inline std::size_t to_size(std::string &s) noexcept { return s.size(); }

inline std::size_t to_size(const char *s) noexcept { return std::strlen(s); }

inline std::size_t to_size(char *s) noexcept { return std::strlen(s); }

template <typename T> inline std::size_t to_size(const T &) noexcept {
  return sizeof(T);
}

// Type trait: is the key a variable-length type?
template <typename Key>
struct is_variable_length_key
    : std::integral_constant<bool,
                             std::is_same<Key, std::string>::value ||
                                 std::is_same<Key, const std::string>::value ||
                                 std::is_same<Key, char *>::value ||
                                 std::is_same<Key, const char *>::value> {};

// --------------------------------------------------------------------------------------------

template <typename Key, typename Mapped, bool queryOnlyExistingKeys = false>
class static_radix_map_node {
public:
  static constexpr std::size_t MAX_SLOTS = 257;
  using byte_t = unsigned char;
  using value_type = MapDataT<Key, Mapped>;
  using TupleVectorT = std::vector<value_type>;
  using KeyBase = std::remove_const_t<Key>;
  using node_t = static_radix_map_node<Key, Mapped, queryOnlyExistingKeys>;

  // Non-copyable
  static_radix_map_node(const static_radix_map_node &) = delete;
  static_radix_map_node &operator=(const static_radix_map_node &) = delete;

  static_radix_map_node(TupleVectorT &data,
                        const std::vector<std::size_t> &nodeIndexes)
      : ndx_(0), data_(data), min_slot_(255), max_slot_(0) {
    if (!nodeIndexes.empty())
      initialize(data, nodeIndexes);
  }

  ~static_radix_map_node() = default;

  void initialize(TupleVectorT &data,
                  const std::vector<std::size_t> &nodeIndexes) {
    ndx_ = this->calc_best_index(nodeIndexes);
    std::vector<std::vector<std::size_t>> slots;
    slots.resize(MAX_SLOTS);
    std::vector<std::size_t> next_stage;

    for (std::size_t i = 0, i_end = nodeIndexes.size(); i < i_end; ++i) {
      int ii = nodeIndexes[i];
      if (node_t::key_size(data[ii]) > ndx_) {
        unsigned short key = static_cast<unsigned short>(
            static_cast<unsigned char>(node_t::key_data(data[ii])[ndx_]));
        slots[key].push_back(ii);
        if (key < min_slot_)
          min_slot_ = key;
        if (key >= max_slot_)
          max_slot_ = key;
      } else // all keys with length less than selected index
        next_stage.push_back(ii);
    }

    std::size_t count = slot_size(min_slot_, max_slot_);
    nodes_.resize(count);

    for (std::size_t i = min_slot_; i <= max_slot_; ++i) {
      insert_slot_data(slots[i], i);
    }
    insert_slot_data(next_stage, max_slot_ + 1);
  }

  void insert_slot_data(const std::vector<std::size_t> &data, int index) {
    if (!data.empty()) {
      int ii = index - min_slot_;
      auto &slot = nodes_[ii];
      if (data.size() > 1) {
        slot.link_ = std::make_unique<node_t>(data_, data);
        slot.tuple_ = nullptr;
      } else {
        slot.tuple_ = &data_[data[0]];
        slot.link_ = nullptr;
      }
    }
  }

  // calculate column with maximum selectivity
  // Uses bitset<256> instead of std::set for efficiency
  std::size_t calc_best_index(const std::vector<std::size_t> &nodeIndexes) {
    if (nodeIndexes.size() == 1)
      return 0;

    // get length of largest/smallest string
    std::size_t max_sz = 0;
    std::size_t min_sz = std::size_t(-1);

    for (std::size_t i = 0, i_end = nodeIndexes.size(); i < i_end; ++i) {
      std::size_t sz = node_t::key_size(data_[nodeIndexes[i]]);
      if (sz > max_sz)
        max_sz = sz;
      if (sz < min_sz)
        min_sz = sz;
    }

    // get a column with maximum selectivity
    // this starts from the end to avoid endless loop
    // for prefix strings sequences. e.g. a, aa
    // where all columns have equal different char counts.
    // to reduce memory consumption prefer columns with
    // small indexes and with small intervals of [min_val, max_val]

    std::size_t min_slot_count = 256;
    std::size_t max_count = 0;
    std::size_t best_ndx = 0;
    for (int i = max_sz - 1; i >= 0; --i) {
      std::bitset<256> seen;
      byte_t lo = 255, hi = 0;
      for (std::size_t j = 0, j_end = nodeIndexes.size(); j < j_end; ++j) {
        int jj = nodeIndexes[j];
        if (node_t::key_size(data_[jj]) > static_cast<std::size_t>(i)) {
          byte_t b = static_cast<byte_t>(node_t::key_data(data_[jj])[i]);
          seen.set(b);
          if (b < lo)
            lo = b;
          if (b > hi)
            hi = b;
        }
      }

      std::size_t slot_count =
          static_cast<std::size_t>(hi) - static_cast<std::size_t>(lo) + 1;
      std::size_t count = seen.count();
      if (count > max_count ||
          (count > 1 && count == max_count && slot_count <= min_slot_count)) {
        min_slot_count = slot_count;
        max_count = count;
        best_ndx = i;
      }
    }

    if (max_count == 1 && best_ndx < min_sz)
      throw std::range_error("static_radix_map::keys are not unique!");

    return best_ndx;
  }

  Mapped value(const Key &key) const {
    const value_type *p = this->tuple(key);
    return p == nullptr ? Mapped() : node_t::value(*p);
  }

  Mapped &value_ref(const Key &key) {
    value_type *p = this->tuple(key);
    if (p != nullptr)
      return node_t::value(*p);
    else
      throw std::runtime_error("static_radix_map::value: key does not exists!");
  }

  int count(const Key &key) const noexcept { return tuple(key) != nullptr; }

  // fixed length types
  const value_type *tuple(const Key &key_param, std::true_type) const noexcept {
    const char *key = to_const_char(key_param);

    std::size_t slot = static_cast<byte_t>(key[ndx_]);
    const ChildEntry *node = (slot >= min_slot_ && slot <= max_slot_)
                                 ? &nodes_[slot - min_slot_]
                                 : nullptr;

    while (node != nullptr && !node->is_empty() && node->is_link()) {
      const node_t *mapNode = node->link_.get();
      std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
      node = (slot >= mapNode->min_slot_ && slot <= mapNode->max_slot_)
                 ? &mapNode->nodes_[slot - mapNode->min_slot_]
                 : nullptr;
    }

    if (node != nullptr && !node->is_empty() && !node->is_link() &&
        std::memcmp(key, node_t::key_data(*node->tuple_), sizeof(Key)) == 0)
      return node->tuple_;
    return nullptr;
  }

  // fixed length types, querying only existing keys
  const value_type *existing_tuple(const Key &key_param,
                                   std::true_type) const noexcept {
    const char *key = to_const_char(key_param);

    std::size_t slot = static_cast<byte_t>(key[ndx_]);
    const ChildEntry *node = &nodes_[slot - min_slot_];

    while (node->is_link()) {
      const node_t *mapNode = node->link_.get();
      std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
      node = &mapNode->nodes_[slot - mapNode->min_slot_];
    }

    return node->tuple_;
  }

  // variable length types like std::string or const char*
  const value_type *tuple(const Key &key_param,
                          std::false_type) const noexcept {
    std::size_t len = to_size(key_param);
    const char *key = to_const_char(key_param);

    const ChildEntry *node = nullptr;
    if (ndx_ < len) {
      std::size_t slot = static_cast<byte_t>(key[ndx_]);
      node = (slot >= min_slot_ && slot <= max_slot_)
                 ? &nodes_[slot - min_slot_]
                 : nullptr;
    } else if (max_slot_ - min_slot_ + 1 < nodes_.size())
      node = &nodes_[max_slot_ - min_slot_ + 1];

    while (node != nullptr && !node->is_empty() && node->is_link()) {
      const node_t *mapNode = node->link_.get();
      if (mapNode->ndx_ < len) {
        std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
        node = (slot >= mapNode->min_slot_ && slot <= mapNode->max_slot_)
                   ? &mapNode->nodes_[slot - mapNode->min_slot_]
                   : nullptr;
      } else if (mapNode->max_slot_ - mapNode->min_slot_ + 1 <
                 mapNode->nodes_.size())
        node = &mapNode->nodes_[mapNode->max_slot_ - mapNode->min_slot_ + 1];
      else
        node = nullptr;
    }

    if (node != nullptr && !node->is_empty() && !node->is_link() &&
        len == node_t::key_size(*node->tuple_)) {
      if (std::memcmp(key, node_t::key_data(*node->tuple_), len) == 0)
        return node->tuple_;
    }
    return nullptr;
  }

  // variable length types, query existing keys
  const value_type *existing_tuple(const Key &key_param,
                                   std::false_type) const noexcept {
    std::size_t len = to_size(key_param);
    const char *key = to_const_char(key_param);

    const ChildEntry *node = nullptr;
    if (ndx_ < len) {
      std::size_t slot = static_cast<byte_t>(key[ndx_]);
      node = &nodes_[slot - min_slot_];
    } else
      node = &nodes_[max_slot_ - min_slot_ + 1];

    while (node->is_link()) {
      const node_t *mapNode = node->link_.get();
      if (mapNode->ndx_ < len) {
        std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
        node = &mapNode->nodes_[slot - mapNode->min_slot_];
      } else
        node = &mapNode->nodes_[mapNode->max_slot_ - mapNode->min_slot_ + 1];
    }

    return node->tuple_;
  }

  const value_type *tuple(const Key &key_param) const noexcept {
    constexpr bool is_fixed = !is_variable_length_key<Key>::value;
    if constexpr (queryOnlyExistingKeys)
      return existing_tuple(key_param,
                            std::integral_constant<bool, is_fixed>{});
    else
      return tuple(key_param, std::integral_constant<bool, is_fixed>{});
  }

  value_type *tuple(const Key &key) noexcept {
    return const_cast<value_type *>(
        static_cast<const node_t *>(this)->tuple(key));
  }

  std::size_t used_mem() const noexcept {
    std::size_t res = sizeof(*this) + nodes_.capacity() * sizeof(ChildEntry);
    for (std::size_t i = 0, i_end = nodes_.size(); i < i_end; ++i) {
      if (nodes_[i].is_link())
        res += nodes_[i].link_->used_mem();
    }
    return res;
  }

  static constexpr std::size_t slot_size(std::size_t min_slot,
                                         std::size_t max_slot) noexcept {
    return (max_slot >= min_slot) ? max_slot - min_slot + 2 : 0;
  }

  // -------------------------------------------------------------------------
  // Flattening logic for contiguous memory layout
  // -------------------------------------------------------------------------

  uint32_t flatten(std::vector<uint32_t> &buffer,
                   const value_type *base_ptr) const {
    std::vector<uint32_t> child_indices;
    std::size_t count = nodes_.size();
    child_indices.reserve(count);

    // Post-order: Process children first so they are already in the buffer
    for (std::size_t i = 0; i < count; ++i) {
      if (nodes_[i].is_empty()) {
        child_indices.push_back(0); // Empty slot
      } else if (nodes_[i].is_link()) {
        // Recursively flatten the child node
        uint32_t offset = nodes_[i].link_->flatten(buffer, base_ptr);
        // Even numbers are Node offsets (shifted), Odd numbers are Leaves.
        child_indices.push_back(offset << 1);
      } else {
        // Leaf: Store index + 1 shifted (to distinguish from offsets)
        // Format: (index << 1) | 1. 0 is reserved for NULL.
        // Even numbers are Node offsets. Odd numbers are Leaves.
        if (nodes_[i].tuple_) {
          std::size_t idx = nodes_[i].tuple_ - base_ptr;
          // Ensure index fits in 31 bits
          if (idx > 0x7FFFFFFF)
            throw std::overflow_error("Too many keys for 32-bit index");

          child_indices.push_back((static_cast<uint32_t>(idx) << 1) | 1);
        } else {
          child_indices.push_back(0);
        }
      }
    }

    // Now capture current buffer size as our start index
    uint32_t my_offset = static_cast<uint32_t>(buffer.size());

    // Header Word 0: ndx (key byte index to check)
    buffer.push_back(static_cast<uint32_t>(ndx_));

    // Header Word 1: min_slot (low 8), max_slot (next 8), unused (high 16)
    uint32_t slots_info = static_cast<uint32_t>(min_slot_) |
                          (static_cast<uint32_t>(max_slot_) << 8);
    buffer.push_back(slots_info);

    // Append child pointers
    buffer.insert(buffer.end(), child_indices.begin(), child_indices.end());

    return my_offset;
  }

  double average_path_length() const noexcept {
    // sum of path lengths over all keys
    using element_t = std::pair<const node_t *, int>;

    std::vector<element_t> stack;
    stack.push_back(std::make_pair(this, 0));
    int res = 0;

    while (!stack.empty()) {
      const element_t &e = stack.back();
      const node_t *node = e.first;
      int deep = e.second;
      stack.pop_back();

      for (std::size_t i = 0; i < node->nodes_.size(); ++i) {
        if (!node->nodes_[i].is_empty()) {
          if (node->nodes_[i].is_link())
            stack.push_back(
                std::make_pair(node->nodes_[i].link_.get(), deep + 1));
          else if (node->nodes_[i].tuple_ != nullptr)
            res += deep;
        }
      }
    }

    return (res * 1.0) / data_.size();
  }

private:
  // Child entry: replaces union + bool with a clean struct using unique_ptr
  struct ChildEntry {
    std::unique_ptr<node_t> link_;
    value_type *tuple_ = nullptr;

    ChildEntry() = default;
    ~ChildEntry() = default;

    // Move only (unique_ptr)
    ChildEntry(ChildEntry &&) noexcept = default;
    ChildEntry &operator=(ChildEntry &&) noexcept = default;

    bool is_empty() const noexcept { return !link_ && !tuple_; }

    bool is_link() const noexcept { return link_ != nullptr; }
  };

  std::size_t ndx_;
  std::vector<ChildEntry> nodes_;
  std::vector<value_type> &data_;
  unsigned short min_slot_;
  unsigned short max_slot_;

  static inline const char *key_data(const value_type &tuple) noexcept {
    return tuple;
  }

  static inline std::size_t key_size(const value_type &tuple) noexcept {
    return tuple.size();
  }

  static inline Mapped &value(value_type &tuple) noexcept {
    return tuple.value();
  }

  static inline const Mapped &value(const value_type &tuple) noexcept {
    return tuple.value();
  }
};

} // namespace detail
} // namespace radix

#endif
