#include <atomic_bitvector.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <random>

using namespace std::chrono_literals;

void test_iterators(size_t n_bits) {
    atomicbitvector::atomic_bv_t a(n_bits);
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<size_t> uniform_dist(0, n_bits-1);
    /*
    for (size_t i = 0; i < n_bits; ++i) {
        a.set(i);
    }
    */
    for (size_t i = 0; i < n_bits/10; ++i) {
        a.set(uniform_dist(e1));
    }
    for (auto i : a) {
        std::cerr << i << " ";
    }
    std::cerr << std::endl;
    exit(1);
}

int main(int argc, char** argv) {
    assert(argc == 3);
    size_t n_bits = std::stol(argv[1]);
    size_t n_threads = std::stol(argv[2]);
    //test_iterators(n_bits); // uncomment to run iterator tests
    std::random_device r;
    std::atomic<bool> done;
    done.store(false);
    atomicbitvector::atomic_bv_t x(n_bits);
    std::thread t([&x,&n_bits,&done](void) {
                      uint64_t set_bits = 0;
                      std::cerr << (double) set_bits / n_bits << "        \r";
                      while (set_bits < n_bits) {
                          set_bits = 0;
                          for (size_t i = 0; i < n_bits; ++i) {
                              set_bits += x.test(i);
                          }
                          std::cerr << (double) set_bits / n_bits << "        \r";
                          std::this_thread::sleep_for(200ms);
                      }
                      done.store(true);
                      std::cerr << std::endl;
                  });
    std::vector<std::thread> writers;
    auto writer_lambda = [&x,&n_bits,&r,&done](uint64_t seed) {
                             std::default_random_engine e1(r());
                             std::uniform_int_distribution<size_t> uniform_dist(0, n_bits-1);
                             while (!done.load()) {
                                 x.set(uniform_dist(e1));
                             }
                         };
    for (size_t i = 0; i < n_threads; ++i) {
        writers.emplace_back(writer_lambda, i);
    }
    t.join();
    for (size_t i = 0; i < n_threads; ++i) {
        writers[i].join();
    }
    return 0;
}

