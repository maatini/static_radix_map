static_radix_map
================

Static_radix_map is a efficient 'static' mapping class. The keys are fixed as a whole and the mapped values are changeable.
Details:
     The implementation is based on radix trees and uses the knowledge of
     all keys to make a (near optimal) multiway tree. Some performance measurements 
     suggest significant improvements over the hash-container boost::unordered_map
     and a free patricia tree implementation.
     The improvement is particularly large over the hash algorithm for small 
     sizes < 1000 and/or querying absent keys.
     Of course the advantages over the hash algorithm diminish as the key count 
     grows because of the O(log n) characteristic of the tree based 
     algorithm against the O(1) hash algorithm. 
     Specializations for variable length types std::string and const char* are 
     implemented. Specialization for const char* added only semantics for those pointers.

     The interface is a subset of std::map and has all methods implemented
     which make sense in static context.

Comment:
     Keys are in same order as feed in. 

Requirements:
    All key-value-pairs needed for initialization
    Key must have the representational equality property, thus 
    equality predicate is memory equivalence and implemented with std::memcmp.
          

Open questions: 
     - the implemented greedy approach may be suboptimal
         
Libraries: boost 1.41 (shared_ptr, tuple, noncopyable, type_traits, mpl, counting_iterator), 
     prior boost versions >= 1.36 should also work             
