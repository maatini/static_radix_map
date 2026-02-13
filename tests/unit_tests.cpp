#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <static_radix_map/static_radix_map.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using namespace radix;

// ---- Basic Tests ----

void test_basic_string_map() {
  std::cout << "Running test_basic_string_map..." << std::endl;
  std::map<std::string, int> data;
  data["apple"] = 1;
  data["banana"] = 2;
  data["cherry"] = 3;

  static_radix_map<std::string, int> smap(data);

  assert(smap.count("apple") == 1);
  assert(smap.count("banana") == 1);
  assert(smap.count("cherry") == 1);
  assert(smap.count("date") == 0);

  assert(smap.value("apple") == 1);
  assert(smap.value("banana") == 2);
  assert(smap.value("cherry") == 3);
  assert(smap.value("date") == 0);
  std::cout << "PASSED" << std::endl;
}

void test_prefix_relationships() {
  std::cout << "Running test_prefix_relationships..." << std::endl;
  std::map<std::string, int> data;
  data["a"] = 1;
  data["aa"] = 2;
  data["aaa"] = 3;
  data["apple"] = 4;
  data["apply"] = 5;

  static_radix_map<std::string, int> smap(data);

  assert(smap.value("a") == 1);
  assert(smap.value("aa") == 2);
  assert(smap.value("aaa") == 3);
  assert(smap.value("apple") == 4);
  assert(smap.value("apply") == 5);
  assert(smap.value("ap") == 0);
  std::cout << "PASSED" << std::endl;
}

struct Point {
  int x, y;
  bool operator<(const Point &o) const {
    return x < o.x || (x == o.x && y < o.y);
  }
};

void test_fixed_length_keys() {
  std::cout << "Running test_fixed_length_keys..." << std::endl;
  std::map<Point, int> data;
  data[{1, 2}] = 100;
  data[{3, 4}] = 200;

  static_radix_map<Point, int> smap(data);

  assert(smap.value({1, 2}) == 100);
  assert(smap.value({3, 4}) == 200);
  assert(smap.value({1, 1}) == 0);
  std::cout << "PASSED" << std::endl;
}

void test_regression_ujzre() {
  std::cout << "Running test_regression_ujzre..." << std::endl;
  std::map<std::string, int> data;
  data["DEY"] = 1;
  data["UJZRE"] = 2;
  data["UW"] = 3;
  data["WUGREJ"] = 4;
  data["YMDREBPRRAJXJ"] = 5;
  data["AIXI"] = 6;

  static_radix_map<std::string, int> smap(data);

  assert(smap.value("DEY") == 1);
  assert(smap.value("UJZRE") == 2);
  assert(smap.value("UW") == 3);
  assert(smap.value("WUGREJ") == 4);
  assert(smap.value("YMDREBPRRAJXJ") == 5);
  assert(smap.value("AIXI") == 6);
  std::cout << "PASSED" << std::endl;
}

void test_empty_map() {
  std::cout << "Running test_empty_map..." << std::endl;
  std::map<std::string, int> data;
  static_radix_map<std::string, int> smap(data);
  assert(smap.count("anything") == 0);
  assert(smap.empty());
  assert(smap.size() == 0);
  assert(smap.begin() == smap.end());
  std::cout << "PASSED" << std::endl;
}

// ---- Iterator Tests ----

void test_iterators() {
  std::cout << "Running test_iterators..." << std::endl;
  std::map<std::string, int> data;
  data["alpha"] = 10;
  data["beta"] = 20;
  data["gamma"] = 30;

  static_radix_map<std::string, int> smap(data);

  // size/empty
  assert(smap.size() == 3);
  assert(!smap.empty());

  // forward iteration counts
  int count = 0;
  for (auto it = smap.begin(); it != smap.end(); ++it) {
    ++count;
  }
  assert(count == 3);

  // const iteration
  const auto &csmap = smap;
  count = 0;
  for (auto it = csmap.cbegin(); it != csmap.cend(); ++it) {
    ++count;
  }
  assert(count == 3);

  // reverse iteration
  count = 0;
  for (auto it = smap.rbegin(); it != smap.rend(); ++it) {
    ++count;
  }
  assert(count == 3);

  // const reverse
  count = 0;
  for (auto it = csmap.crbegin(); it != csmap.crend(); ++it) {
    ++count;
  }
  assert(count == 3);

  // range-based for
  count = 0;
  for (const auto &kv : smap) {
    (void)kv;
    ++count;
  }
  assert(count == 3);

  std::cout << "PASSED" << std::endl;
}

// ---- find & equal_range Tests ----

void test_find() {
  std::cout << "Running test_find..." << std::endl;
  std::map<std::string, int> data;
  data["one"] = 1;
  data["two"] = 2;
  data["three"] = 3;

  static_radix_map<std::string, int> smap(data);

  auto it = smap.find("two");
  assert(it != smap.end());
  assert(it->value() == 2);

  auto it2 = smap.find("four");
  assert(it2 == smap.end());

  // const find
  const auto &csmap = smap;
  auto cit = csmap.find("one");
  assert(cit != csmap.end());
  assert(cit->value() == 1);

  std::cout << "PASSED" << std::endl;
}

void test_equal_range() {
  std::cout << "Running test_equal_range..." << std::endl;
  std::map<std::string, int> data;
  data["x"] = 1;
  data["y"] = 2;

  static_radix_map<std::string, int> smap(data);

  auto range = smap.equal_range("x");
  assert(range.first != smap.end());
  assert(std::distance(range.first, range.second) == 1);
  assert(range.first->value() == 1);

  auto range2 = smap.equal_range("z");
  assert(range2.first == smap.end());
  assert(range2.first == range2.second);

  std::cout << "PASSED" << std::endl;
}

// ---- Operator Tests ----

void test_subscript_operator() {
  std::cout << "Running test_subscript_operator..." << std::endl;
  std::map<std::string, int> data;
  data["key1"] = 100;
  data["key2"] = 200;

  static_radix_map<std::string, int> smap(data);

  // Valid access
  assert(smap["key1"] == 100);
  assert(smap["key2"] == 200);

  // Modify via operator[]
  smap["key1"] = 999;
  assert(smap["key1"] == 999);

  // Invalid access throws
  bool threw = false;
  try {
    smap["nonexistent"];
  } catch (const std::runtime_error &) {
    threw = true;
  }
  assert(threw);

  // Const operator[] throws for absent key
  const auto &csmap = smap;
  threw = false;
  try {
    csmap["absent"];
  } catch (const std::runtime_error &) {
    threw = true;
  }
  assert(threw);

  std::cout << "PASSED" << std::endl;
}

void test_comparison_operators() {
  std::cout << "Running test_comparison_operators..." << std::endl;
  std::map<std::string, int> data1;
  data1["a"] = 1;
  data1["b"] = 2;

  std::map<std::string, int> data2;
  data2["a"] = 1;
  data2["b"] = 2;

  std::map<std::string, int> data3;
  data3["c"] = 3;

  static_radix_map<std::string, int> smap1(data1);
  static_radix_map<std::string, int> smap2(data2);
  static_radix_map<std::string, int> smap3(data3);

  assert(smap1 == smap2);
  assert(!(smap1 != smap2));
  assert(smap1 != smap3);

  std::cout << "PASSED" << std::endl;
}

// ---- Move Semantics Test ----

void test_move_semantics() {
  std::cout << "Running test_move_semantics..." << std::endl;
  std::map<std::string, int> data;
  data["apple"] = 1;
  data["banana"] = 2;

  static_radix_map<std::string, int> smap1(data);
  assert(smap1.size() == 2);

  // Move construct
  static_radix_map<std::string, int> smap2(std::move(smap1));
  assert(smap2.size() == 2);
  assert(smap2.value("apple") == 1);
  assert(smap2.value("banana") == 2);

  // Move assign
  std::map<std::string, int> data2;
  data2["cherry"] = 3;
  static_radix_map<std::string, int> smap3(data2);
  smap3 = std::move(smap2);
  assert(smap3.size() == 2);
  assert(smap3.value("apple") == 1);

  std::cout << "PASSED" << std::endl;
}

// ---- Swap Test ----

void test_swap() {
  std::cout << "Running test_swap..." << std::endl;
  std::map<std::string, int> data1;
  data1["a"] = 1;

  std::map<std::string, int> data2;
  data2["b"] = 2;
  data2["c"] = 3;

  static_radix_map<std::string, int> smap1(data1);
  static_radix_map<std::string, int> smap2(data2);

  smap1.swap(smap2);
  assert(smap1.size() == 2);
  assert(smap1.value("b") == 2);
  assert(smap2.size() == 1);
  assert(smap2.value("a") == 1);

  std::cout << "PASSED" << std::endl;
}

// ---- Clear Test ----

void test_clear() {
  std::cout << "Running test_clear..." << std::endl;
  std::map<std::string, int> data;
  data["x"] = 1;

  static_radix_map<std::string, int> smap(data);
  assert(!smap.empty());
  smap.clear();
  assert(smap.empty());
  assert(smap.size() == 0);
  assert(smap.count("x") == 0);

  std::cout << "PASSED" << std::endl;
}

// ---- Single Key Test ----

void test_single_key() {
  std::cout << "Running test_single_key..." << std::endl;
  std::map<std::string, int> data;
  data["only"] = 42;

  static_radix_map<std::string, int> smap(data);
  assert(smap.size() == 1);
  assert(smap.value("only") == 42);
  assert(smap.count("only") == 1);
  assert(smap.count("other") == 0);
  assert(smap.value("other") == 0);

  std::cout << "PASSED" << std::endl;
}

// ---- Empty String Key Test ----

void test_empty_string_key() {
  std::cout << "Running test_empty_string_key..." << std::endl;
  std::map<std::string, int> data;
  data[""] = 100;
  data["a"] = 200;

  static_radix_map<std::string, int> smap(data);
  assert(smap.value("") == 100);
  assert(smap.value("a") == 200);
  assert(smap.count("") == 1);
  assert(smap.count("b") == 0);

  std::cout << "PASSED" << std::endl;
}

// ---- Long Keys Test ----

void test_long_keys() {
  std::cout << "Running test_long_keys..." << std::endl;
  std::map<std::string, int> data;
  std::string long1(1000, 'a');
  std::string long2(1000, 'b');
  std::string long3 = long1;
  long3[999] = 'x'; // differ only in last char

  data[long1] = 1;
  data[long2] = 2;
  data[long3] = 3;

  static_radix_map<std::string, int> smap(data);
  assert(smap.value(long1) == 1);
  assert(smap.value(long2) == 2);
  assert(smap.value(long3) == 3);

  std::cout << "PASSED" << std::endl;
}

// ---- used_mem Test ----

void test_used_mem() {
  std::cout << "Running test_used_mem..." << std::endl;
  std::map<std::string, int> data;
  data["a"] = 1;
  data["b"] = 2;

  static_radix_map<std::string, int> smap(data);
  assert(smap.used_mem() > 0);
  assert(smap.used_mem() > sizeof(smap));

  std::cout << "PASSED" << std::endl;
}

// ---- Stress Test ----

void test_random_stress() {
  std::cout << "Running test_random_stress..." << std::endl;
  const int n = 5000;
  std::vector<std::string> keys;
  std::set<std::string> key_set;
  std::default_random_engine generator(42);
  std::uniform_int_distribution<int> len_dist(1, 20);
  std::uniform_int_distribution<int> char_dist('A', 'Z');

  while (key_set.size() < n) {
    int len = len_dist(generator);
    std::string s;
    for (int j = 0; j < len; ++j)
      s += (char)char_dist(generator);
    key_set.insert(s);
  }
  keys.assign(key_set.begin(), key_set.end());

  std::map<std::string, int> data;
  for (int i = 0; i < n; ++i)
    data[keys[i]] = i + 1;

  static_radix_map<std::string, int> smap(data);

  // All present keys must be found with correct values
  for (auto const &pair : data) {
    if (smap.count(pair.first) == 0) {
      std::cerr << "FAILURE: Key not found: " << pair.first << std::endl;
      assert(false);
    }
    if (smap.value(pair.first) != pair.second) {
      std::cerr << "FAILURE: Wrong value for " << pair.first << ": got "
                << smap.value(pair.first) << " expected " << pair.second
                << std::endl;
      assert(false);
    }
  }

  // Test absent keys don't give false positives
  std::uniform_int_distribution<int> absent_len(1, 25);
  std::uniform_int_distribution<int> absent_char('a', 'z'); // lowercase
  for (int i = 0; i < 1000; ++i) {
    int len = absent_len(generator);
    std::string s;
    for (int j = 0; j < len; ++j)
      s += (char)absent_char(generator);
    if (key_set.count(s) == 0) {
      assert(smap.count(s) == 0);
      assert(smap.value(s) == 0);
    }
  }

  std::cout << "PASSED" << std::endl;
}

int main() {
  test_basic_string_map();
  test_prefix_relationships();
  test_fixed_length_keys();
  test_regression_ujzre();
  test_empty_map();
  test_iterators();
  test_find();
  test_equal_range();
  test_subscript_operator();
  test_comparison_operators();
  test_move_semantics();
  test_swap();
  test_clear();
  test_single_key();
  test_empty_string_key();
  test_long_keys();
  test_used_mem();
  test_random_stress();
  std::cout << "\nAll tests passed!" << std::endl;
  return 0;
}
