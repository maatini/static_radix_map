#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <static_radix_map/static_radix_map.hpp>

using namespace radix;

class Timer {
public:
  Timer() : start_(std::chrono::high_resolution_clock::now()) {}

  void reset() { start_ = std::chrono::high_resolution_clock::now(); }

  double seconds() const {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - start_;
    return diff.count();
  }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

std::vector<std::string> generateTestKeys(int n, int min_len = 1,
                                          int max_len = 16) {
  std::default_random_engine generator(42); // fixed seed for reproducibility
  std::uniform_int_distribution<int> len_distribution(min_len, max_len);
  std::uniform_int_distribution<int> char_distribution('A', 'Z');

  std::cerr << "Generating " << n << " keys...";
  std::set<std::string> res;
  while (static_cast<int>(res.size()) < n) {
    int m = len_distribution(generator);
    std::string s;
    for (int j = 0; j < m; ++j)
      s += static_cast<char>(char_distribution(generator));
    res.insert(s);
  }
  std::cerr << " done\n";
  return std::vector<std::string>(res.begin(), res.end());
}

std::vector<std::string>
generateAbsentKeys(int n, const std::set<std::string> &existingKeys) {
  std::default_random_engine generator(999);
  std::uniform_int_distribution<int> len_dist(1, 16);
  std::uniform_int_distribution<int> char_dist('a', 'z'); // lowercase
  std::vector<std::string> result;
  result.reserve(n);
  while (static_cast<int>(result.size()) < n) {
    int m = len_dist(generator);
    std::string s;
    for (int j = 0; j < m; ++j)
      s += static_cast<char>(char_dist(generator));
    if (existingKeys.count(s) == 0)
      result.push_back(std::move(s));
  }
  return result;
}

template <class MapT>
static inline int get_value(MapT &m, const std::string &s) {
  auto iter = m.find(s);
  return (iter != m.end()) ? iter->second : 0;
}

template <class M>
double map_perf_test(M &the_map, const std::vector<std::string> &keys,
                     const std::vector<size_t> &indices,
                     const std::string &caption) {
  const int loops = 10;
  int dummy_sum = 0;

  // Warm-up to fill caches and stabilize CPU clock
  for (size_t idx : indices) {
    dummy_sum += get_value(the_map, keys[idx]);
  }

  double total_time = 0;
  for (int k = 0; k < loops; ++k) {
    int sum = 0;
    Timer t;
    for (size_t idx : indices) {
      sum += get_value(the_map, keys[idx]);
    }
    total_time += t.seconds();
    dummy_sum += sum; // prevent optimization
  }

  double avg_time = total_time / loops;
  std::cout << "--> " << std::setw(22) << std::left << caption
            << " avg time: " << std::fixed << std::setprecision(4) << avg_time
            << "s (sum check " << dummy_sum << ")" << std::endl;
  return avg_time;
}

// Absent-key benchmark: only absent keys are queried
template <class M>
double absent_key_test(M &the_map, const std::vector<std::string> &absent_keys,
                       const std::string &caption) {
  const int loops = 10;
  int dummy_sum = 0;

  // Warm-up
  for (const auto &k : absent_keys)
    dummy_sum += get_value(the_map, k);

  double total_time = 0;
  for (int i = 0; i < loops; ++i) {
    int sum = 0;
    Timer t;
    for (const auto &k : absent_keys)
      sum += get_value(the_map, k);
    total_time += t.seconds();
    dummy_sum += sum;
  }

  double avg_time = total_time / loops;
  std::cout << "--> " << std::setw(22) << std::left << caption
            << " absent avg: " << std::fixed << std::setprecision(4) << avg_time
            << "s (sum " << dummy_sum << ")" << std::endl;
  return avg_time;
}

struct BenchResult {
  int wins = 0;
  double total_time = 0;
  double total_absent = 0;
};

void performance_test(std::map<std::string, BenchResult> &results, int n,
                      int tries = 10000000) {
  auto keys = generateTestKeys(n);
  std::set<std::string> key_set(keys.begin(), keys.end());
  auto absent = generateAbsentKeys(tries / 10, key_set);

  std::default_random_engine generator(123);
  std::uniform_int_distribution<int> val_dist(1, 100000);
  std::uniform_int_distribution<size_t> index_dist(0, n - 1);

  std::map<std::string, int> data;
  for (int i = 0; i < n; ++i)
    data[keys[i]] = val_dist(generator);

  // Measure construction time
  Timer build_timer;
  static_radix_map<std::string, int, false> smap(data);
  double build_time = build_timer.seconds();

  std::unordered_map<std::string, int> umap(data.begin(), data.end());
  std::map<std::string, int> omap(data.begin(), data.end());

  // Pre-generate indices
  std::vector<size_t> indices;
  indices.reserve(tries);
  for (int i = 0; i < tries; ++i)
    indices.push_back(index_dist(generator));

  std::cout << "\nTesting " << n << " keys (" << (tries / 1000000)
            << "M searches)..." << std::endl;
  std::cout << "static_map memory: " << std::fixed << std::setprecision(2)
            << (smap.used_mem() / 1024.0) << " KB" << std::endl;
  std::cout << "construction time: " << std::fixed << std::setprecision(6)
            << build_time << "s" << std::endl;

  const char *const STATICMAP = "static_radix_map";
  const char *const UMAP = "std::unordered_map";
  const char *const OMAP = "std::map";

  // Test order swapped per size to mitigate order bias
  double ut, st, ot;
  if (n % 2 == 0) {
    ut = map_perf_test(umap, keys, indices, UMAP);
    ot = map_perf_test(omap, keys, indices, OMAP);
    st = map_perf_test(smap, keys, indices, STATICMAP);
  } else {
    st = map_perf_test(smap, keys, indices, STATICMAP);
    ot = map_perf_test(omap, keys, indices, OMAP);
    ut = map_perf_test(umap, keys, indices, UMAP);
  }

  // Absent-key test
  std::cout << "  Absent-key test (" << absent.size() << " queries):\n";
  absent_key_test(umap, absent, UMAP);
  absent_key_test(omap, absent, OMAP);
  absent_key_test(smap, absent, STATICMAP);

  results[STATICMAP].total_time += st;
  results[UMAP].total_time += ut;
  results[OMAP].total_time += ot;
  if (st < ut && st < ot)
    results[STATICMAP].wins++;
  else if (ut < st && ut < ot)
    results[UMAP].wins++;
  else
    results[OMAP].wins++;
}

int main() {
  std::map<std::string, BenchResult> results;

  std::vector<int> sizes = {16, 64, 256, 1024, 5000, 10000};
  for (int n : sizes) {
    performance_test(results, n);
  }

  std::cout << "\nOverall Statistics:\n";
  for (auto const &entry : results) {
    std::cout << std::setw(22) << std::left << entry.first << ": "
              << entry.second.wins << " wins, " << std::fixed
              << std::setprecision(4) << entry.second.total_time
              << "s total time\n";
  }

  return 0;
}
