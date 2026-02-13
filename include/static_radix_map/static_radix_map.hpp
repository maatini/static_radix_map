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
//
// File:    static_radix_map.hpp, header only template implementation
// Date:    06/08/2010
// Purpose:
//       a very efficient 'static' mapping class. The keys are fixed as a whole
//       and the mapped values are changeable
//
// Details:
//       The implementation is based on radix trees and uses the knowledge of
//       all keys to make a (near optimal) multiway tree. Some performance
//       measurements suggest significant improvements over the hash-container
//       std::unordered_map. The improvement is particularly large for small
//       sizes < 1000 and/or querying absent keys.
//       Of course the advantages over the hash algorithm diminish as the key
//       count grows because of the O(log n) characteristic of the tree based
//       algorithm against the O(1) hash algorithm.
//       Specializations for variable length types std::string and const char*
//       are implemented. Specialization for const char* added only semantics
//       for those pointers.
//
//       The interface is a subset of std::map and has all methods implemented
//       which make sense in static context.
//
// Comment:
//       Keys are in same order as feed in.
//
// Requirements:
//       All key-value-pairs needed for initialization
//       Key must have the representational equality property, thus
//       equality predicate is memory equivalence and implemented with
//       std::memcmp.
//
// Dependencies: C++17 standard library only (no external dependencies)
//
//----------------------------------------------------------------------------

#ifndef STATIC_RADIX_MAP_HPP

#define STATIC_RADIX_MAP_HPP

#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "static_radix_map_node.hpp"

namespace radix {

template <typename Key, typename Mapped, bool queryOnlyExistingKeys = false>
class static_radix_map {
public:
  using key_type = Key;
  using mapped_type = Mapped;
  using size_type = std::size_t;
  using map_type = static_radix_map<Key, Mapped, queryOnlyExistingKeys>;
  using node_type =
      detail::static_radix_map_node<Key, Mapped, queryOnlyExistingKeys>;
  using value_type = typename node_type::value_type;

  using iterator = typename std::vector<value_type>::iterator;
  using const_iterator = typename std::vector<value_type>::const_iterator;

  using reverse_iterator = typename std::vector<value_type>::reverse_iterator;
  using const_reverse_iterator =
      typename std::vector<value_type>::const_reverse_iterator;

  template <class Map> explicit static_radix_map(const Map &m) {
    init_map(m.begin(), m.end());
  }

  template <typename InputIterator>
  static_radix_map(InputIterator start, InputIterator end) {
    init_map(start, end);
  }

  // Move support
  static_radix_map(static_radix_map &&) noexcept = default;
  static_radix_map &operator=(static_radix_map &&) noexcept = default;

  // Copy support
  static_radix_map(const static_radix_map &) = default;
  static_radix_map &operator=(const static_radix_map &other) {
    if (this != &other) {
      map_type tmp(other);
      this->swap(tmp);
    }
    return *this;
  }

  // returns Mapped() for non existing keys
  Mapped value(const Key &key) const noexcept {
    const value_type *p = lookup(key);
    return p == nullptr ? Mapped() : p->value();
  }

  // throws runtime_error for non existing keys
  Mapped &operator[](const Key &key) {
    value_type *p = const_cast<value_type *>(lookup(key));
    if (p != nullptr)
      return p->value();
    else
      throw std::runtime_error("static_radix_map::value: key does not exists!");
  }

  const Mapped &operator[](const Key &key) const {
    const value_type *p = lookup(key);
    if (p != nullptr)
      return p->value();
    else
      throw std::runtime_error("static_radix_map::value: key does not exists!");
  }

  template <bool query_only_existing_keys>
  bool operator==(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const noexcept {
    return keyValues_ == other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator!=(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const noexcept {
    return keyValues_ != other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator<(const static_radix_map<Key, Mapped, query_only_existing_keys>
                     &other) const noexcept {
    return keyValues_ < other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator>(const static_radix_map<Key, Mapped, query_only_existing_keys>
                     &other) const noexcept {
    return keyValues_ > other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator<=(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const noexcept {
    return keyValues_ <= other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator>=(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const noexcept {
    return keyValues_ >= other.keyValues_;
  }

  // count returns 1 for existing keys otherwise 0
  std::size_t count(const Key &key) const noexcept {
    return lookup(key) != nullptr;
  }

  // iterators are realized via delegation
  iterator begin() noexcept { return keyValues_.begin(); }
  iterator end() noexcept { return keyValues_.end(); }

  const_iterator begin() const noexcept { return keyValues_.begin(); }
  const_iterator end() const noexcept { return keyValues_.end(); }

  const_iterator cbegin() const noexcept { return keyValues_.cbegin(); }
  const_iterator cend() const noexcept { return keyValues_.cend(); }

  reverse_iterator rbegin() noexcept { return keyValues_.rbegin(); }
  reverse_iterator rend() noexcept { return keyValues_.rend(); }

  const_reverse_iterator rbegin() const noexcept { return keyValues_.rbegin(); }
  const_reverse_iterator rend() const noexcept { return keyValues_.rend(); }

  const_reverse_iterator crbegin() const noexcept {
    return keyValues_.crbegin();
  }
  const_reverse_iterator crend() const noexcept { return keyValues_.crend(); }

  const_iterator find(const key_type &key) const noexcept {
    const value_type *p = lookup(key);
    if (p != nullptr)
      return begin() + (p - &keyValues_[0]);
    else
      return end();
  }

  iterator find(const key_type &key) noexcept {
    const value_type *p = lookup(key);
    if (p != nullptr)
      return begin() + (p - &keyValues_[0]);
    else
      return end();
  }

  void swap(map_type &other) noexcept {
    std::swap(keyValues_, other.keyValues_);
    std::swap(treeBuffer_, other.treeBuffer_);
    std::swap(rootOffset_, other.rootOffset_);
  }

  bool empty() const noexcept { return keyValues_.empty(); }

  std::pair<iterator, iterator> equal_range(const key_type &key) noexcept {
    iterator iter = find(key);
    return (iter != end()) ? std::make_pair(iter, iter + 1)
                           : std::make_pair(iter, iter);
  }

  std::pair<const_iterator, const_iterator>
  equal_range(const key_type &key) const noexcept {
    const_iterator iter = find(key);
    return (iter != end()) ? std::make_pair(iter, iter + 1)
                           : std::make_pair(iter, iter);
  }

  void clear() noexcept {
    keyValues_.clear();
    treeBuffer_.clear();
    rootOffset_ = 0;
  }

  size_type size() const noexcept { return keyValues_.size(); }

  size_type max_size() const noexcept { return keyValues_.max_size(); }

  // used memory in bytes
  std::size_t used_mem() const noexcept {
    return sizeof(*this) + keyValues_.capacity() * sizeof(value_type) +
           treeBuffer_.capacity() * sizeof(uint32_t);
  }

private:
  std::vector<value_type> keyValues_;
  std::vector<uint32_t> treeBuffer_;
  uint32_t rootOffset_ = 0;

  const value_type *lookup(const Key &key_param) const noexcept {
    if (treeBuffer_.empty())
      return nullptr;

    using namespace detail;
    const char *key = to_const_char(key_param);
    std::size_t len = to_size(key_param);

    const uint32_t *tree = treeBuffer_.data();
    uint32_t curr = rootOffset_;

    while (true) {
      uint32_t ndx = tree[curr];
      uint32_t slots_info = tree[curr + 1];
      uint32_t min = slots_info & 0xFF;
      uint32_t max = (slots_info >> 8) & 0xFF;

      uint32_t child_val = 0;

      if (ndx < len) {
        // Optimized range check: (byte - min) <= (max - min) handles both >=
        // min and <= max because of unsigned underflow if byte < min.
        uint32_t diff = static_cast<uint8_t>(key[ndx]) - min;
        if (diff <= (max - min)) {
          child_val = tree[curr + 2 + diff];
        }
      } else {
        child_val = tree[curr + 2 + (max - min + 1)];
      }

      if (child_val == 0) {
        return nullptr; // Empty slot
      }

      if (child_val & 1) {
        // Leaf
        size_t idx = child_val >> 1;
        const value_type &candidate = keyValues_[idx];

        if (to_size(candidate.first) == len &&
            std::memcmp(key, to_const_char(candidate.first), len) == 0) {
          return &candidate;
        }
        return nullptr;
      } else {
        // Node
        curr = child_val >> 1;
      }
    }
  }

  template <typename InputIterator>
  void init_map(InputIterator start, InputIterator end) {
    // pre-process data
    std::size_t sz = std::distance(start, end);
    keyValues_.reserve(sz);
    for (auto iter = start; iter != end; ++iter) {
      keyValues_.push_back(value_type(iter->first, iter->second));
    }

    treeBuffer_.clear();
    treeBuffer_.push_back(0); // Sentinel to ensure all valid offsets are > 0

    if (sz > 0) {
      // initial selection are all keys for root node
      std::vector<std::size_t> selection(sz);
      std::iota(selection.begin(), selection.end(), 0);
      node_type nodeTree(keyValues_, selection);
      rootOffset_ = nodeTree.flatten(treeBuffer_, &keyValues_[0]);
    }
  }
};

} // namespace radix

#endif
