#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <static_radix_map/static_radix_map.hpp>
#include <string>
#include <vector>

using namespace static_map_stuff;

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
  // This specific set of keys was known to trigger the logic bug
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
  std::cout << "PASSED" << std::endl;
}

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
  std::cout << "PASSED" << std::endl;
}

int main() {
  test_basic_string_map();
  test_prefix_relationships();
  test_fixed_length_keys();
  test_regression_ujzre();
  test_empty_map();
  test_random_stress();
  std::cout << "\nAll tests passed!" << std::endl;
  return 0;
}
