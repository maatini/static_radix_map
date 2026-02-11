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
//       boost::unordered_map and a free patricia tree implementation. The
//       improvement is particularly large over the hash algorithm for small
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
//
// Open questions:
//       - the implemented greedy approach may be suboptimal
//
// Libraries: boost 1.41 (shared_ptr, tuple, noncopyable, type_traits, mpl,
// counting_iterator),
//       prior boost versions >= 1.36 should also work
//----------------------------------------------------------------------------

#ifndef STATIC_RADIX_MAP_HPP

#define STATIC_RADIX_MAP_HPP

#include <cstdlib> // for size_t
#include <stdexcept>
#include <utility>
#include <vector>

#include "boost/iterator/counting_iterator.hpp"
#include "boost/shared_ptr.hpp"

#include "static_radix_map_node.hpp"

namespace static_map_stuff {

template <typename Key, typename Mapped, bool queryOnlyExistingKeys = false>
class static_radix_map {
public:
  typedef Key key_type;
  typedef Mapped mapped_type;
  typedef std::size_t size_type;
  typedef static_radix_map<Key, Mapped, queryOnlyExistingKeys> map_type;
  typedef
      typename detail::static_radix_map_node<Key, Mapped, queryOnlyExistingKeys>
          node_type;
  typedef typename node_type::value_type value_type;

  typedef typename std::vector<value_type>::iterator iterator;
  typedef typename std::vector<value_type>::const_iterator const_iterator;

  typedef typename std::vector<value_type>::reverse_iterator reverse_iterator;
  typedef typename std::vector<value_type>::const_reverse_iterator
      const_reverse_iterator;

  template <class Map> static_radix_map(const Map &m) {
    init_map(m.begin(), m.end());
  }

  template <typename iterator> static_radix_map(iterator start, iterator end) {
    init_map(start, end);
  }

  // returns Mapped() for non existing keys
  Mapped value(const Key &key) const {
    const value_type *p = lookup(key);
    return p == 0 ? Mapped() : p->value();
  }

  // throws runtime_error for non existing keys
  Mapped &operator[](const Key &key) {
    value_type *p = const_cast<value_type *>(lookup(key));
    if (p != 0)
      return p->value();
    else
      throw std::runtime_error("static_radix_map::value: key does not exists!");
  }

  const Mapped &operator[](const Key &key) const {
    const value_type *p = lookup(key);
    if (p != 0)
      return p->value();
    else
      throw std::runtime_error("static_radix_map::value: key does not exists!");
  }

  map_type &operator=(const map_type &other) {
    if (this != &other) {
      map_type tmp(other);
      this->swap(tmp);
    }
    return *this;
  }

  template <bool query_only_existing_keys>
  bool operator==(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const {
    return keyValues_ == other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator!=(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const {
    return keyValues_ != other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator<(const static_radix_map<Key, Mapped, query_only_existing_keys>
                     &other) const {
    return keyValues_ < other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator>(const static_radix_map<Key, Mapped, query_only_existing_keys>
                     &other) const {
    return keyValues_ > other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator<=(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const {
    return keyValues_ <= other.keyValues_;
  }

  template <bool query_only_existing_keys>
  bool operator>=(const static_radix_map<Key, Mapped, query_only_existing_keys>
                      &other) const {
    return keyValues_ >= other.keyValues_;
  }

  // count returns 1 for existing keys otherwise 0
  std::size_t count(const Key &key) const { return lookup(key) != 0; }

  // iterators are realized via delegation
  iterator begin() { return keyValues_.begin(); }

  iterator end() { return keyValues_.end(); }

  const_iterator begin() const { return keyValues_.begin(); }

  const_iterator end() const { return keyValues_.end(); }

  reverse_iterator rbegin() { return keyValues_.rbegin(); }

  reverse_iterator rend() { return keyValues_.rend(); }

  const_reverse_iterator rbegin() const { return keyValues_.rbegin(); }

  const_reverse_iterator rend() const { return keyValues_.rend(); }

  const_iterator find(const key_type &key) const {
    const value_type *p = lookup(key);
    if (p != 0)
      return begin() + (p - &keyValues_[0]);
    else
      return end();
  }

  iterator find(const key_type &key) {
    const value_type *p = lookup(key);
    if (p != 0)
      return begin() + (p - &keyValues_[0]);
    else
      return end();
  }

  void swap(map_type &other) {
    // never throws an exception
    std::swap(keyValues_, other.keyValues_);
    std::swap(treeBuffer_, other.treeBuffer_);
    std::swap(rootOffset_, other.rootOffset_);
  }

  bool empty() const { return keyValues_.empty(); }

  std::pair<iterator, iterator> equal_range(const key_type &key) {
    iterator iter = find(key);
    return (iter != end()) ? std::make_pair(iter, iter + 1)
                           : std::make_pair(iter, iter);
  }

  std::pair<const_iterator, const_iterator>
  equal_range(const key_type &key) const {
    const_iterator iter = find(key);
    return (iter != end()) ? std::make_pair(iter, iter + 1)
                           : std::make_pair(iter, iter);
  }

  void clear() {
    keyValues_.clear();
    treeBuffer_.clear();
    rootOffset_ = 0;
  }

  size_type size() const { return keyValues_.size(); }

  size_type max_size() const { return keyValues_.max_size(); }

  // used memory in bytes
  std::size_t used_mem() const {
    return sizeof(*this) + keyValues_.capacity() * sizeof(value_type) +
           treeBuffer_.capacity() * sizeof(uint32_t);
  }

private:
  std::vector<value_type> keyValues_;
  std::vector<uint32_t> treeBuffer_;
  uint32_t rootOffset_;

  const value_type *lookup(const Key &key_param) const {
    if (treeBuffer_.empty())
      return 0;

    using namespace detail;
    const char *key = to_const_char(key_param);
    std::size_t len = to_size(key_param);

    uint32_t curr = rootOffset_;
    while (true) {
      if (curr >= treeBuffer_.size()) {
        return 0; // Should not happen
      }

      uint32_t ndx = treeBuffer_[curr];
      uint32_t slots_info = treeBuffer_[curr + 1];
      uint32_t min = slots_info & 0xFF;
      uint32_t max = (slots_info >> 8) & 0xFF;

      uint32_t child_val = 0;

      if (ndx < len) {
        uint8_t byte = static_cast<uint8_t>(key[ndx]);
        if (byte >= min && byte <= max) {
          child_val = treeBuffer_[curr + 2 + (byte - min)];
        }
      } else {
        child_val = treeBuffer_[curr + 2 + (max - min + 1)];
      }

      if (child_val == 0) {
        return 0; // Empty slot
      }

      if (child_val & 1) {
        // Leaf
        size_t idx = child_val >> 1;
        const value_type &candidate = keyValues_[idx];

        if (to_size(candidate.first) == len &&
            std::memcmp(key, to_const_char(candidate.first), len) == 0) {
          return &candidate;
        }
        return 0;
      } else {
        // Node
        curr = child_val;
      }
    }
  }

  template <typename iterator> void init_map(iterator start, iterator end) {
    // pre-process data
    std::size_t sz = std::distance(start, end);
    keyValues_.reserve(sz);
    for (iterator iter = start; iter != end; ++iter) {
      keyValues_.push_back(value_type(iter->first, iter->second));
    }

    if (sz > 0) {
      // initial selection are all keys for root node
      std::vector<std::size_t> selection(
          boost::counting_iterator<std::size_t>(0),
          boost::counting_iterator<std::size_t>(sz));
      node_type nodeTree(keyValues_, selection);
      rootOffset_ = nodeTree.flatten(treeBuffer_, &keyValues_[0]);
    }
  }
};

} // namespace static_map_stuff

#endif
