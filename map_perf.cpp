#include <cmath>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/times.h>
#endif

#include <iostream>
#include <iomanip>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "static_radix_map.hpp"

#define    FOR(i, m, n)    for(int i=static_cast<int>(m); i<static_cast<int>(n); ++i)
#define    REP(i, n)        FOR(i, 0, n)
#define    ALL(x)            (x).begin(), (x).end()

typedef std::vector<std::string> VS;

using namespace static_map_stuff;

void perf_startup() {
   SetThreadAffinityMask(GetCurrentThread(), 1);
   SetThreadIdealProcessor(GetCurrentThread(), 0);
   Sleep(1);
}

class MongoTimer {
public:
    MongoTimer() 
        : start_(clock())
    {}

    double seconds() const {
        return clock()-start_;
    }

    double reset() {
        double now =  clock();
        double res = now-start_;
        start_ = now;
        return res;
    }

    double clock() const {
        double thistime;
#ifdef WIN32
        thistime = GetTickCount()/1000.0;
#else
        struct timeval now;
        gettimeofday(&now,NULL);
        thistime = now.tv_sec +    now.tv_usec/1000000.0;
#endif
        return thistime;
    } 
private:
    double start_;
};

#ifdef WIN32

class performance_timer
{
public:
    performance_timer()
        : start_(0)
        , finish_(0)
    {
        query_frequency();
        start();
    }

    void start()
    {
        LARGE_INTEGER large;
        QueryPerformanceCounter(&large);
        start_ = large.QuadPart;
    }

    void finish()
    {
        LARGE_INTEGER large;
        QueryPerformanceCounter(&large);
        finish_ = large.QuadPart;
    }

    double seconds() 
    {
      finish();
        return
            static_cast<double>(finish_ - start_) /
            static_cast<double>(frequency_);
    }

    double reset() {
      finish();
      double res = seconds();
        start();
        return res;
    }

private:
    static void query_frequency()
    {
        if (frequency_ == 0)
        {
            LARGE_INTEGER large;
            QueryPerformanceFrequency(&large);
            frequency_ = large.QuadPart;
        }
    }

    static __int64 frequency_;
    __int64 start_;
    __int64 finish_;
};

__int64 performance_timer::frequency_ = 0;

#else
   typedef MongoTimer performance_timer;
#endif

std::vector<std::string> generateTestKeys(int n, int min_len = 1, int max_len = 16)
{
	std::default_random_engine generator;
	std::uniform_int_distribution<int> len_distribution(min_len, max_len);
	std::uniform_int_distribution<int> char_distribution('A', 'Z');

	std::cerr << "start generating test data..";
    std::set<std::string> res;

    while(res.size() < n) {
		int m = len_distribution(generator);
        std::string s;
        REP(j, m)
			s += std::string(1, static_cast<char>(char_distribution(generator)));

        res.insert(s);
    }
    std::cerr << "done\n";
    return std::vector<std::string>(res.begin(), res.end());
}

// ---------------------------------------------------------------------------

template<class MapT>
static inline int get_value(MapT& m, const std::string& s) {
   auto iter(m.find(s));
   if(iter != m.end())
      return iter->second;
         
    return 0;
}

double round(double x, int decimals) {
   int f = 1;
   while(decimals) {
      f *= 10;
      --decimals;
   }
   return static_cast<int>(x*f+0.5)/static_cast<double>(f);
}

template<class M>
double map_perf_test(M& the_map, const std::vector<std::string>& keys, int m, const std::string& caption) {
   const int loops = 3;
   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution(0, keys.size() - 1);

   int sum = 0;
   int n = keys.size();

   performance_timer mt;
   REP(k, loops) {
      REP(i, m) {
		 int ndx = distribution(generator);
         sum += get_value(the_map, keys[ndx]);
      }
   }
   double res = mt.reset();
   std::cout << "-->" << std::setw(26) << std::left << caption << "  time:" << round(res/loops, 2) << " sum:" << sum << std::endl;
   return res;
}

void performance_test(std::map<std::string, std::pair<int, double> >& wins, int n, int m = 0, int tries = 10000000) {
    // generate test data
    auto keys = generateTestKeys(n);
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(1, 100000);
	
	std::map<std::string, int> data;
    REP(i, n) {
		int v = distribution(generator);
        data[keys[i]] = v;
    }

    // initialize maps
   static_radix_map<std::string, int, false> smap_absent(data);
   static_radix_map<std::string, int, true> smap_existing(data);

   std::cout << "staticmap size is:" << smap_absent.used_mem() << std::endl;
   std::unordered_map<std::string, int> umap;

   std::for_each(data.begin(), data.end(), [&umap](const std::pair<const std::string, int>& p) {
      umap[p.first] = p.second;
   });

   // complete test set with some random keys
   auto rnd_keys = generateTestKeys(m);
   REP(i, m) {
	   keys.push_back(rnd_keys[i]);
   }

   // checking mapped values
   REP(i, n) {
      if(umap[keys[i]] != smap_absent.value(keys[i])) {
         smap_absent.value(keys[i]);
         std::cerr << "keys[i] -> " << umap[keys[i]] << " smap, ->" << smap_absent.value(keys[i]) << " smap\n";
      }
   }

    // do performance-check 
    std::cout 
        << std::setw(10) << std::left << n << " keys\n" 
        << std::setw(10) << std::left << m << " not keys\n" 
        << std::setw(10) << std::left << tries << " loops\n" << std::endl;

   const char* const STATICMAP = "static_map";
   const char* const BOOSTHASH = "std::unordered_map";

   double ut = map_perf_test(umap, keys, tries, BOOSTHASH);

   double st;
   if(m >= 0)
      st = map_perf_test(smap_absent, keys, tries, STATICMAP);
   else
      st = map_perf_test(smap_existing, keys, tries, STATICMAP);

   std::cout << "\n\n";

   wins[STATICMAP].second += st;
   wins[BOOSTHASH].second += ut;

   std::vector<std::pair<double, std::string> > t2v;
   t2v.push_back(std::make_pair(st, STATICMAP));
   t2v.push_back(std::make_pair(ut, BOOSTHASH));
   
   std::sort(t2v.begin(), t2v.end());
   double min_time = t2v[0].first;
   std::for_each(t2v.begin(), t2v.end(), [&wins, min_time](const std::pair<double, std::string>& p) {
      wins[p.second].first += p.first == min_time;
   });
}

template<typename MapT, typename key_type>
double do_test_std(MapT& my_map, std::vector<key_type>& v, int probes, int n) {
   int elements = v.size();
   int loops = probes/elements;
   double sum = 0;
   for(int k=0; k < n; ++k) {
      auto i_end = my_map.end();
      for(int i =0; i < loops; ++i) {
         for(int j = 0; j < elements; ++j) {
            auto iter = my_map.find(v[j]);
            if(iter != i_end)
               sum += iter->second;
         }
      }
   }

   return sum;
}

template<typename T, bool query_only_existing_keys>
void test_type_map(int m, int n, int absent = 0) {

   typedef T key_type;

   // generate test data
   std::default_random_engine generator;
   std::uniform_int_distribution<int> distribution((std::numeric_limits<T>::min)(), (std::numeric_limits<T>::max)());

   std::unordered_map<key_type, key_type> umap;

   std::vector<key_type> v;
   v.reserve(m+absent);
   while(v.size() < m) {
      key_type x;
      char* xx = reinterpret_cast<char*>(&x);
      REP(i, sizeof(key_type)) 
		  *xx++ = distribution(generator) % 256;

      if(umap.count(x) == 0) {
         int i = v.size();
         umap[x] = (i+1)%m;
         v.push_back(x);
      }
   }

   while(v.size() < m+absent) {
      key_type x;
      char* xx = reinterpret_cast<char*>(&x);
      REP(i, sizeof(key_type)) 
		  *xx++ = distribution(generator) % 256;

      if(umap.count(x) == 0) {
         v.push_back(x);
      }
   }

   std::random_shuffle(v.begin(), v.end());

   // commence performance tests

   static_radix_map<key_type, key_type, query_only_existing_keys> smap(ALL(umap));
   performance_timer mt;
   const int N = 2;
   {
      mt.reset();
      double sum = do_test_std(smap, v, n, N);
      double time = round(mt.reset()/N, 2);
      std::cout << std::setw(5) << m << " static map: " << std::setw(12) << sum << " time:" << time << std::endl;
   }

   {
      mt.reset();
      double sum = do_test_std(umap, v, n, N);
      double time = round(mt.reset()/N, 2);
      std::cout << std::setw(5) << std::left << m << "       umap: " << std::setw(12) << sum << " time:" << time << std::endl;
   }

   std::cout << "\n" << std::endl;
}

template<typename T, bool query_only_existing_keys>
void test_type(int absent) {
   const int N = 100000000;
   test_type_map<T, query_only_existing_keys>(16, N, absent);
   test_type_map<T, query_only_existing_keys>(32, N, absent);
   test_type_map<T, query_only_existing_keys>(64, N, absent);
   test_type_map<T, query_only_existing_keys>(128, N, absent);
   test_type_map<T, query_only_existing_keys>(256, N, absent);
   test_type_map<T, query_only_existing_keys>(512, N, absent);
   test_type_map<T, query_only_existing_keys>(1024, N, absent);
   test_type_map<T, query_only_existing_keys>(2000, N, absent);
   test_type_map<T, query_only_existing_keys>(5000, N, absent);
   test_type_map<T, query_only_existing_keys>(10000, N, absent);
   test_type_map<T, query_only_existing_keys>(20000, N, absent);
}

void performance() {
   std::map<std::string, std::pair<int, double>> wins;

   performance_test(wins, 16);
   performance_test(wins, 32);
   performance_test(wins, 64);
   performance_test(wins, 128);
   
   performance_test(wins, 256);
   performance_test(wins, 512);
   performance_test(wins, 1024);
   performance_test(wins, 5000);
   performance_test(wins, 10000);
   /*
   performance_test(wins, 16, 10);
   performance_test(wins, 128, 10);
   performance_test(wins, 512, 10);
   performance_test(wins, 1024, 10);
   performance_test(wins, 5000, 10);
   performance_test(wins, 10000, 10);

   performance_test(wins, 16, 1000);
   performance_test(wins, 128, 1000);
   performance_test(wins, 512, 1000);
   performance_test(wins, 1024, 1000);
   performance_test(wins, 5000, 1000);
   performance_test(wins, 10000, 1000);
   */

   std::cout << "statistics:\n";
   for(auto iter=wins.begin(), i_end=wins.end(); iter != i_end; ++iter) 
      std::cout 
         << std::setw(26) << std::left << iter->first
         << std::setw(4) << std::left << iter->second.first << " wins " 
         << std::setw(4) << std::left << iter->second.second << " seconds total time " 
         << std::endl;
}

int main() {
   perf_startup();
   try {
      //test_type<int16_t, false>(0);
      performance();
      
   }
   catch(std::exception& e) {
      std::cerr << e.what() << "\n";
   }

}

