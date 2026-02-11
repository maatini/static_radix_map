//----------------------------------------------------------------------------
// 				
// Copyright (c) 2010 Martin Richardt
//
//      email: martin.richardt@web.de
//
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of 
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all 
// copies, substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//----------------------------------------------------------------------------



#ifndef STATIC_MAP_RADIX_NODE_HPP

#define STATIC_MAP_RADIX_NODE_HPP

#include <cstdlib> // for size_t
#include <cstring> // for strlen
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/mpl/end.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/vector.hpp>
#include "boost/noncopyable.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/type_traits.hpp"
#include "boost/tuple/tuple.hpp"


namespace static_map_stuff {

   namespace detail {

      // Map data abstraction. 
      // Key must have the representational equality property

      template<class Key, class Mapped>
      struct MapDataT : public std::pair<Key, Mapped> {
         MapDataT(const Key& key, const Mapped& value)
            : std::pair<Key, Mapped>(key, value)
         {}

         MapDataT(const std::pair<Key, Mapped>& p)
            : std::pair<Key, Mapped>(p)
         {}

         std::size_t size() const {
            return sizeof(Key);
         }

         operator const char*() const {
            return reinterpret_cast<const char*>(&key());
         }

         const Key& key() const {
            return std::pair<Key, Mapped>::first;
         }

         Mapped& value() {
            return std::pair<Key, Mapped>::second;
         }

         const Mapped& value() const {
            return std::pair<Key, Mapped>::second;
         }
      };

      // specialization for std::string
      template<class Mapped>
      struct MapDataT<std::string, Mapped> : public std::pair<std::string, Mapped> {
         MapDataT(const std::string& key, const Mapped& value)
            : std::pair<std::string, Mapped>(key, value)
         {}

         MapDataT(const std::pair<std::string, Mapped>& p)
            : std::pair<std::string, Mapped>(p)
         {}

         std::size_t size() const {
            return std::pair<std::string, Mapped>::first.size();
         }

         operator const char*() const {
            return key().c_str();
         }

         const std::string& key() const {
            return std::pair<std::string, Mapped>::first;
         }

         Mapped& value() {
            return std::pair<std::string, Mapped>::second;
         }

         const Mapped& value() const {
            return std::pair<std::string, Mapped>::second;
         }
      };

      // specialization for const char*
      template<class Mapped>
      struct MapDataT<const char*, Mapped> : public std::pair<const char*, Mapped> {
         std::size_t size_;

         MapDataT(const char* key, const Mapped& value)
            : std::pair<const char*, Mapped>(key, value)
            , size_(std::strlen(key))
         {}

         MapDataT(const std::pair<const char*, Mapped>& p)
            : std::pair<const char*, Mapped>(p)
            , size_(std::strlen(p.first))
         {}

         std::size_t size() const {
            return size_;
         }

         operator const char*() const {
            return std::pair<const char*, Mapped>::first;
         }

         const char* key() const {
            return std::pair<const char*, Mapped>::first;
         }

         Mapped& value() {
            return std::pair<const char*, Mapped>::second;
         }

         const Mapped& value() const {
            return std::pair<const char*, Mapped>::second;
         }
      };

      // ----------------------------

      const char* to_const_char(std::string& s) {
         return s.c_str();
      }

      const char* to_const_char(const std::string& s) {
         return s.c_str();
      }

      const char* to_const_char(const char* s) {
         return s;
      }

      template<typename T> 
      const char* to_const_char(const T& x) {
         return reinterpret_cast<const char*>(&x);
      }

      std::size_t to_size(const std::string& s) {
         return s.size();
      }

      std::size_t to_size(std::string& s) {
         return s.size();
      }

      std::size_t to_size(const char* s) {
         return std::strlen(s);
      }

      std::size_t to_size(char* s) {
         return std::strlen(s);
      }

      // --------------------------------------------------------------------------------------------

      template<typename Key, typename Mapped, bool queryOnlyExistingKeys = false>
      class static_radix_map_node : boost::noncopyable  
      {
      public:
         static const std::size_t MAX_SLOTS = 257;
         typedef unsigned char byte_t;
         typedef MapDataT<Key, Mapped> value_type;

         typedef std::vector<value_type>	TupleVectorT;
         typedef typename boost::remove_const<Key>::type	KeyBase;
         typedef static_radix_map_node<Key, Mapped, queryOnlyExistingKeys> node_t;

         static_radix_map_node(TupleVectorT& data, const std::vector<std::size_t>& nodeIndexes) 
            : ndx_(0)
            , nodes_(0)
            , data_(data)     
            , min_slot_(255)
            , max_slot_(0)

         {
            if(!nodeIndexes.empty())
               initialize(data, nodeIndexes);
         }

         ~static_radix_map_node() {
            clear();
         }

         void clear() {
            for(std::size_t i = 0, i_end = slot_size(min_slot_, max_slot_); i < i_end; ++i) 
               delete nodes_[i];
            delete[] nodes_;
            ndx_ = MAX_SLOTS;
         }

         void initialize(TupleVectorT& data, const std::vector<std::size_t>& nodeIndexes) {
            ndx_ = this->calc_best_index(nodeIndexes);
            std::vector<std::vector<std::size_t> > slots;
            slots.resize(MAX_SLOTS);
            std::vector<std::size_t> next_stage;

            for(std::size_t i = 0, i_end = nodeIndexes.size(); i < i_end; ++i) {
               int ii = nodeIndexes[i];
               if(node_t::key_size(data[ii]) > ndx_) {
                  unsigned short key = static_cast<unsigned short>(static_cast<unsigned char>(node_t::key_data(data[ii])[ndx_]));
                  slots[key].push_back(ii);
                  if(key < min_slot_)
                     min_slot_ = key;
                  if(key >= max_slot_)
                     max_slot_ = key;
               }
               else // all keys with length less than selected index
                  next_stage.push_back(ii);
            }

            int slot_count = slot_size(min_slot_, max_slot_);
            nodes_ = new NodeT*[slot_count];
            std::fill(nodes_, nodes_+slot_count, static_cast<NodeT*>(0));

            for(std::size_t i = min_slot_; i <= max_slot_; ++i) {
               insert_slot_data(slots[i], i);
            }
            insert_slot_data(next_stage, max_slot_+1);
         }

         void insert_slot_data(const std::vector<std::size_t>& data, int index) {
            if(!data.empty()) {
               int ii = index-min_slot_;
               nodes_[ii] = new NodeT();
               nodes_[ii]->isLink_ = data.size() > 1;
               if(nodes_[ii]->isLink_) {
                  nodes_[ii]->data_.link_ = new node_t(data_, data);
               }
               else 
                  nodes_[ii]->data_.tuple_ = &data_[data[0]];
            }
         }

         // calculate column with maximum selectivity
         std::size_t calc_best_index(const std::vector<std::size_t>& nodeIndexes) {
            if(nodeIndexes.size() == 1)
               return 0;

            // get length of largest string 
            std::size_t max_sz = 0;
            std::size_t min_sz = std::size_t(-1);

            for(std::size_t i = 0, i_end = nodeIndexes.size(); i < i_end; ++i) {
               std::size_t sz = node_t::key_size(data_[nodeIndexes[i]]);
               if(sz > max_sz) 
                  max_sz = sz;
               if(sz < min_sz) 
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
            for(int i = max_sz-1; i >=0; --i) {
               std::set<byte_t> chars;			
               for(std::size_t j = 0, j_end = nodeIndexes.size(); j < j_end; ++j) {
                  int jj = nodeIndexes[j];
                  if(node_t::key_size(data_[jj]) > static_cast<std::size_t>(i)) {
                     chars.insert(static_cast<byte_t>(node_t::key_data(data_[jj])[i]));
                  }
               }

               std::size_t slot_count = static_cast<std::size_t>(*--chars.end()) - static_cast<std::size_t>(*chars.begin())+1;
               std::size_t count = chars.size();
               if(count > max_count || (count > 1 && count == max_count && slot_count <= min_slot_count)) {
                  min_slot_count = slot_count;
                  max_count = count;
                  best_ndx = i;
               }
            }

            if(max_count == 1 && best_ndx < min_sz) 
               throw std::range_error("static_radix_map::keys are not unique!");

            return best_ndx;
         }

         Mapped value(const Key& key) const {
            const value_type* p = this->tuple(key);
            return p == 0 ? Mapped() : node_t::value(*p);
         }

         Mapped& value_ref(const Key& key) {
            value_type* p = this->tuple(key);
            if(p != 0)
               return node_t::value(*p);
            else 
               throw std::runtime_error("static_radix_map::value: key does not exists!");
         }

         int count(const Key& key) const {
            return tuple(key) != 0;
         }

         // fixed length types
         const value_type* tuple(const Key& key_param, boost::mpl::true_) const {	    
            const char* key = to_const_char(key_param);

            std::size_t slot = static_cast<byte_t>(key[ndx_]);
            const NodeT* node = (slot >= min_slot_ && slot <= max_slot_) ? nodes_[slot-min_slot_] : 0;

            while(node  != 0 && node->isLink_) {
               const node_t* mapNode = node->data_.link_;
               std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
               node = (slot >= mapNode->min_slot_ && slot <= mapNode->max_slot_) ? mapNode->nodes_[slot-mapNode->min_slot_] : 0;
            }

            if(node != 0 && std::memcmp(key, node_t::key_data(*node->data_.tuple_), sizeof(Key)) == 0)
               return node->data_.tuple_;
            return 0;
         }

         // fixed length types, querying only existing keys
         const value_type* existing_tuple(const Key& key_param, boost::mpl::true_) const {	    
            const char* key = to_const_char(key_param);

            std::size_t slot = static_cast<byte_t>(key[ndx_]);
            const NodeT* node = nodes_[slot-min_slot_];

            while(node->isLink_) {
               const node_t* mapNode = node->data_.link_;
               std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
               node = mapNode->nodes_[slot-mapNode->min_slot_];
            }

            return node->data_.tuple_;
         }

         // variable length types like std::string or const char*
         const value_type* tuple(const Key& key_param, boost::mpl::false_) const {	    
            std::size_t len = to_size(key_param);
            const char* key = to_const_char(key_param);

            const NodeT* node = 0;
            if(ndx_ < len) {
               std::size_t slot = static_cast<byte_t>(key[ndx_]);
               node = (slot >= min_slot_ && slot <= max_slot_) ? nodes_[slot-min_slot_] : 0;
            }
            else 
               node = nodes_[max_slot_-min_slot_+1];

            while(node  != 0 && node->isLink_) {
               const node_t* mapNode = node->data_.link_;
               if(mapNode->ndx_ < len) {
                  std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
                  node = (slot >= mapNode->min_slot_ && slot <= mapNode->max_slot_) ? mapNode->nodes_[slot-mapNode->min_slot_] : 0;
               }
               else  
                  node = mapNode->nodes_[mapNode->max_slot_ - mapNode->min_slot_+1];
            }

            if(node != 0 && len == node_t::key_size(*node->data_.tuple_)) {
               if(std::memcmp(key, node_t::key_data(*node->data_.tuple_), len) == 0)
                  return node->data_.tuple_;
            }
            return 0;
         }

         // variable length types like std::string or const char*, query existing keys
         const value_type* existing_tuple(const Key& key_param, boost::mpl::false_) const {	    
            std::size_t len = to_size(key_param);
            const char* key = to_const_char(key_param);

            const NodeT* node = 0;
            if(ndx_ < len) {
               std::size_t slot = static_cast<byte_t>(key[ndx_]);
               node = nodes_[slot-min_slot_];
            }
            else 
               node = nodes_[max_slot_-min_slot_+1];

            while(node->isLink_) {
               node_t* mapNode = node->data_.link_;
               if(mapNode->ndx_ < len) {
                  std::size_t slot = static_cast<byte_t>(key[mapNode->ndx_]);
                  node = mapNode->nodes_[slot-mapNode->min_slot_];
               }
               else  
                  node = mapNode->nodes_[mapNode->max_slot_ - mapNode->min_slot_+1];
            }

            return node->data_.tuple_;
         }

         const value_type* tuple(const Key& key_param) const {	   
            typedef boost::mpl::vector<std::string, const std::string, char*, const char*>	variable_length_types;
            typedef boost::mpl::end<variable_length_types>::type end_type;
            if(queryOnlyExistingKeys) 
               return 
                  existing_tuple(
                     key_param, 
                     typename boost::is_same<
                        typename boost::mpl::find<variable_length_types, Key>::type, end_type
                     >::type()
                  );
            else
               return 
                  tuple(
                     key_param, 
                     typename boost::is_same<
                        typename boost::mpl::find<variable_length_types, Key>::type, end_type
                     >::type()
                  );
         }

         value_type* tuple(const Key& key) {	
            return const_cast<value_type*>(static_cast<const node_t*>(this)->tuple(key));
         }

         std::size_t used_mem() const {
            std::size_t res = sizeof(*this) + slot_size(min_slot_, max_slot_)*sizeof(NodeT*);
            for(std::size_t i = 0, i_end = slot_size(min_slot_, max_slot_); i < i_end; ++i) {
               if(nodes_[i] != 0) 
                  res += nodes_[i]->size();
            }

            return res;
         }

         static inline std::size_t slot_size(std::size_t min_slot, std::size_t max_slot) {	   
            return (max_slot > min_slot) ? max_slot-min_slot+2 : 0;
         }

         double average_path_length() const {
            // sum of path lengths over all keys
            typedef std::pair<const node_t*, int> element_t;

            std::vector<element_t> stack;
            stack.push_back(std::make_pair(this, 0));
            int res = 0;

            while(!stack.empty()) {
               const element_t& e = stack.back();
               const node_t* node = e.first;
               int deep = e.second;
               stack.pop_back();

               for(int i = node->min_slot_; i <= node->max_slot_; ++i) {
                  int j = i-node->min_slot_;
                  if(node->nodes_[j] != 0) {
                     const NodeT* n = node->nodes_[j];
                     if(n->isLink_)
                        stack.push_back(std::make_pair(n->data_.link_, deep+1));
                     else if(n->data_.tuple_ != 0) 
                        res += deep;
                  }
               }
            }

            return (res*1.0)/data_.size();
         }


      private:

         struct NodeT {
            NodeT() 
            {
               data_.link_ = 0;
               isLink_ = false;
            }

            ~NodeT() {
               if(isLink_)
                  delete data_.link_;					
            }

            std::size_t size() const {
               std::size_t res = sizeof(NodeT);
               if(isLink_)
                  res += data_.link_->used_mem();

               return res;
            }

            union Link {
               node_t* link_;
               value_type* tuple_;
            };

            bool isLink_;
            Link data_;
         };

         std::size_t ndx_;
         NodeT** nodes_;
         std::vector<value_type>& data_;
         unsigned short min_slot_;
         unsigned short max_slot_;

         static inline const char* key_data(const value_type& tuple) {
            return tuple;
         }

         static inline std::size_t key_size(const value_type& tuple) {
            return tuple.size();
         }

         static inline Mapped& value(value_type& tuple) {
            return tuple.value();
         }

         static inline const Mapped& value(const value_type& tuple) {
            return tuple.value();
         }
      };

   } // namespace detail
} // namespace static_map_stuff

#endif


