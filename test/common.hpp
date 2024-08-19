#pragma once

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "external/doctest/doctest/doctest.h"

#include <iostream>
#include <vector>
#include <random>

#include "essentials.hpp"

namespace bits::test {

/*
    Generate [sequence_length] random integers, distributed as a poisson RV with a random mean.
*/
std::vector<uint64_t> get_sequence(const uint64_t sequence_length) {
    uint64_t seed = essentials::get_random_seed();
    srand(seed);
    double mean = rand() % 1000;
    std::mt19937 rng(seed);
    std::poisson_distribution<uint64_t> distr(mean);
    std::vector<uint64_t> seq(sequence_length);
    std::generate(seq.begin(), seq.end(), [&]() { return distr(rng); });
    return seq;
}

/*
    Generate a sorted sequence of [sequence_length] random integers,
    distributed as a poisson RV with a random mean.
*/
std::vector<uint64_t> get_sorted_sequence(const uint64_t sequence_length) {
    uint64_t seed = essentials::get_random_seed();
    srand(seed);
    double mean = rand() % 1000;
    std::mt19937 rng(seed);
    std::poisson_distribution<uint64_t> distr(mean);
    uint64_t universe = 0;
    std::vector<uint64_t> seq(sequence_length);
    std::generate(seq.begin(), seq.end(), [&]() {
        uint64_t val = distr(rng);
        universe += val;
        return universe;
    });
    assert(seq.back() == universe);
    assert(std::is_sorted(seq.begin(), seq.end()));
    return seq;
}

}  // namespace bits::test

using namespace bits;
using namespace bits::test;