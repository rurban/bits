#include "common.hpp"
#include "include/bit_vector.hpp"

constexpr uint64_t sequence_length = 10000;

TEST_CASE("bit_vector::builder::get_bits") {
    const uint64_t max_int = test::get_random_uint();
    std::cout << "max_int = " << max_int << std::endl;
    std::vector<uint64_t> seq = test::get_sequence(sequence_length, max_int);
    const uint64_t width = max_int != 0 ? std::ceil(std::log2(max_int + 1)) : 1;
    std::cout << "width = " << width << std::endl;
    bit_vector::builder bv_builder(sequence_length * width);
    for (uint64_t i = 0; i != sequence_length; ++i) {
        bv_builder.set_bits(i * width, seq[i], width);
    }
    bit_vector bv;
    bv_builder.build(bv);
    std::cout << "checking correctness of bit_vector::builder::get_bits..." << std::endl;
    for (uint64_t i = 0; i != sequence_length; ++i) {
        uint64_t got = bv.get_bits(i * width, width);  // get the integer at position i
        uint64_t expected = seq[i];
        REQUIRE_MESSAGE(got == expected, i << "/" << sequence_length << ": got " << got
                                           << " but expected " << expected);
    }
    std::cout << "EVERYTHING OK!" << std::endl;
}

TEST_CASE("bit_vector::iterator::take") {
    const uint64_t max_int = test::get_random_uint();
    std::cout << "max_int = " << max_int << std::endl;
    std::vector<uint64_t> seq = test::get_sequence(sequence_length, max_int);
    const uint64_t width = max_int != 0 ? std::ceil(std::log2(max_int + 1)) : 1;
    std::cout << "width = " << width << std::endl;
    bit_vector::builder bv_builder(sequence_length * width);
    for (uint64_t i = 0; i != sequence_length; ++i) {
        bv_builder.set_bits(i * width, seq[i], width);
    }
    bit_vector bv;
    bv_builder.build(bv);
    std::cout << "checking correctness of bit_vector::iterator::take..." << std::endl;
    for (uint64_t i = 0; i != sequence_length; ++i) {
        bit_vector::iterator it = bv.get_iterator_at(i * width);
        uint64_t got = it.take(width);
        uint64_t expected = seq[i];
        REQUIRE_MESSAGE(got == expected, i << "/" << sequence_length << ": got " << got
                                           << " but expected " << expected);
    }
    std::cout << "EVERYTHING OK!" << std::endl;
}

TEST_CASE("bit_vector::iterator::next") {
    const uint64_t width = test::get_random_uint(100) + 1;  // at least 1
    std::cout << "width = " << width << std::endl;
    bit_vector::builder bv_builder(sequence_length * width);
    for (uint64_t i = 0; i != sequence_length; ++i) {
        bv_builder.set(i * width);  // ones are spaced apart by width
    }
    bit_vector bv;
    bv_builder.build(bv);
    std::cout << "checking correctness of bit_vector::iterator::next..." << std::endl;

    bit_vector::iterator it = bv.begin();
    for (uint64_t i = 0; i != sequence_length; ++i) {
        uint64_t got = it.next();
        uint64_t expected = i * width;
        REQUIRE(got + 1 == it.position());
        REQUIRE_MESSAGE(got == expected, i << "/" << sequence_length << ": got " << got
                                           << " but expected " << expected);
    }

    for (uint64_t i = 0; i != sequence_length - 1; ++i) {
        bit_vector::iterator it = bv.get_iterator_at(i * width + 1);
        uint64_t got = it.next();
        uint64_t expected = (i + 1) * width;
        REQUIRE_MESSAGE(got == expected, i << "/" << sequence_length << ": got " << got
                                           << " but expected " << expected);
    }

    std::cout << "EVERYTHING OK!" << std::endl;
}

TEST_CASE("bit_vector::iterator::prev") {
    const uint64_t width = test::get_random_uint(100) + 1;  // at least 1
    std::cout << "width = " << width << std::endl;
    bit_vector::builder bv_builder(sequence_length * width);
    for (uint64_t i = 0; i != sequence_length; ++i) {
        bv_builder.set(i * width);  // ones are spaced apart by width
    }
    bit_vector bv;
    bv_builder.build(bv);
    std::cout << "checking correctness of bit_vector::iterator::prev..." << std::endl;

    bit_vector::iterator it = bv.get_iterator_at(bv.num_bits() - 1);
    uint64_t pos = it.position();
    for (uint64_t i = 0; i != sequence_length; ++i) {
        uint64_t got = it.prev(pos);
        uint64_t expected = ((sequence_length - i) - 1) * width;
        REQUIRE_MESSAGE(got == expected, i << "/" << sequence_length << ": got " << got
                                           << " but expected " << expected);
        pos = got - 1;
    }

    for (uint64_t i = 1; i != sequence_length; ++i) {
        bit_vector::iterator it = bv.get_iterator_at(i * width - 1);
        uint64_t got = it.prev(it.position());
        uint64_t expected = (i - 1) * width;
        REQUIRE_MESSAGE(got == expected, i << "/" << sequence_length << ": got " << got
                                           << " but expected " << expected);
    }
    std::cout << "EVERYTHING OK!" << std::endl;
}