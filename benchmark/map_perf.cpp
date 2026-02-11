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

#define FOR(i, m, n)                                                           \
  for (int i = static_cast<int>(m); i < static_cast<int>(n); ++i)
#define REP(i, n) FOR(i, 0, n)
#define ALL(x) (x).begin(), (x).end()

using namespace static_map_stuff;

void perf_startup() {
#ifdef WIN32
  SetThreadAffinityMask(GetCurrentThread(), 1);
  SetThreadIdealProcessor(GetCurrentThread(), 0);
#endif
}

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
  while (res.size() < n) {
    int m = len_distribution(generator);
    std::string s;
    REP(j, m)
    s += static_cast<char>(char_distribution(generator));
    res.insert(s);
  }
  std::cerr << " done\n";
  return std::vector<std::string>(res.begin(), res.end());
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
  const int loops = 5;
  int dummy_sum = 0;

  // Warm-up to fill caches and stabilize CPU clock
  for (size_t idx : indices) {
    dummy_sum += get_value(the_map, keys[idx]);
  }

  double total_time = 0;
  REP(k, loops) {
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

void performance_test(std::map<std::string, std::pair<int, double>> &wins,
                      int n, int tries = 10000000) {
  auto keys = generateTestKeys(n);
  std::default_random_engine generator(123);
  std::uniform_int_distribution<int> val_dist(1, 100000);
  std::uniform_int_distribution<size_t> index_dist(0, n - 1);

  std::map<std::string, int> data;
  REP(i, n) data[keys[i]] = val_dist(generator);

  static_radix_map<std::string, int, false> smap(data);
  std::unordered_map<std::string, int> umap(data.begin(), data.end());

  // Pre-generate indices to remove RNG and distribution overhead from timing
  std::vector<size_t> indices;
  indices.reserve(tries);
  REP(i, tries) indices.push_back(index_dist(generator));

  std::cout << "\nTesting " << n << " keys (" << (tries / 1000000)
            << "M searches)..." << std::endl;
  std::cout << "static_map memory: " << std::fixed << std::setprecision(2)
            << (smap.used_mem() / 1024.0) << " KB" << std::endl;

  const char *const STATICMAP = "static_map";
  const char *const UMAP = "std::unordered_map";

  // Test order swapped per size to mitigate order bias in statistics over time
  double ut, st;
  if (n % 2 == 0) {
    ut = map_perf_test(umap, keys, indices, UMAP);
    st = map_perf_test(smap, keys, indices, STATICMAP);
  } else {
    st = map_perf_test(smap, keys, indices, STATICMAP);
    ut = map_perf_test(umap, keys, indices, UMAP);
  }

  wins[STATICMAP].second += st;
  wins[UMAP].second += ut;
  if (st < ut)
    wins[STATICMAP].first++;
  else
    wins[UMAP].first++;
}

int main() {
  perf_startup();
  std::map<std::string, std::pair<int, double>> wins;

  std::vector<int> sizes = {16, 64, 256, 1024, 5000, 10000};
  for (int n : sizes) {
    performance_test(wins, n);
  }

  std::cout << "\nOverall Statistics:\n";
  for (auto const &entry : wins) {
    std::cout << std::setw(20) << std::left << entry.first << ": "
              << entry.second.first << " wins, " << std::fixed
              << std::setprecision(4) << entry.second.second
              << "s total time\n";
  }

  return 0;
}
