#pragma once

#include <vector>
#include <cmath>
#include <iterator>

#include "essentials.hpp"

namespace bits {

struct compact_vector  //
{
    template <typename Vec>
    struct enumerator {
        using iterator_category = std::random_access_iterator_tag;

        enumerator() {}

        enumerator(Vec const* vec, uint64_t i)
            : m_i(i)
            , m_cur_val(0)
            , m_cur_block((i * vec->m_width) >> 6)
            , m_cur_shift((i * vec->m_width) & 63)
            , m_vec(vec)  //
        {
            if (i >= m_vec->size()) return;
            read();
        }

        uint64_t operator*() { return m_cur_val; }

        enumerator& operator++() {
            ++m_i;
            read();
            return *this;
        }

        enumerator& operator--() {
            --m_i;
            enumerator copy(m_vec, m_i);
            *this = copy;
            return *this;
        }

        enumerator operator+(uint64_t jump) {
            enumerator copy(m_vec, m_i + jump);
            return copy;
        }

        enumerator operator-(uint64_t jump) {
            assert(m_i >= jump);
            enumerator copy(m_vec, m_i - jump);
            return copy;
        }

        bool operator==(enumerator const& other) const { return m_i == other.m_i; }
        bool operator!=(enumerator const& other) const { return !(*this == other); }

    private:
        uint64_t m_i;
        uint64_t m_cur_val;
        uint64_t m_cur_block;
        int64_t m_cur_shift;
        Vec const* m_vec;

        void read() {
            if (m_cur_shift + m_vec->m_width <= 64) {
                m_cur_val = m_vec->m_data[m_cur_block] >> m_cur_shift & m_vec->m_mask;
            } else {
                uint64_t res_shift = 64 - m_cur_shift;
                m_cur_val = (m_vec->m_data[m_cur_block] >> m_cur_shift) |
                            (m_vec->m_data[m_cur_block + 1] << res_shift & m_vec->m_mask);
                ++m_cur_block;
                m_cur_shift = -res_shift;
            }

            m_cur_shift += m_vec->m_width;

            if (m_cur_shift == 64) {
                m_cur_shift = 0;
                ++m_cur_block;
            }
        }
    };

    struct builder {
        builder() : m_size(0), m_width(0), m_mask(0), m_back(0), m_cur_block(0), m_cur_shift(0) {}

        builder(uint64_t n, uint64_t w) { resize(n, w); }

        /*
            Resize the container to hold n values, each of width w.
        */
        void resize(uint64_t n, uint64_t w) {
            m_size = n;
            m_width = w;
            m_mask = -(w == 64) | ((uint64_t(1) << w) - 1);
            m_back = 0;
            m_data.resize(
                /* use 1 word more for safe access() */
                essentials::words_for(m_size * m_width) + 1, 0);
        }

        template <typename Iterator>
        builder(Iterator begin, uint64_t n, uint64_t w) : builder(n, w) {
            fill(begin, n);
        }

        template <typename Iterator>
        void fill(Iterator begin, uint64_t n) {
            if (m_width == 0) throw std::runtime_error("width must be > 0");
            for (uint64_t i = 0; i != n; ++i, ++begin) set(i, *begin);
        }

        /*
            Set value v at position i.
        */
        void set(uint64_t i, uint64_t v) {
            assert(m_width != 0);
            assert(i < m_size);
            if (i == m_size - 1) m_back = v;

            uint64_t pos = i * m_width;
            uint64_t block = pos >> 6;
            uint64_t shift = pos & 63;

            m_data[block] &= ~(m_mask << shift);
            m_data[block] |= v << shift;

            uint64_t res_shift = 64 - shift;
            if (res_shift < m_width) {
                m_data[block + 1] &= ~(m_mask >> res_shift);
                m_data[block + 1] |= v >> res_shift;
            }
        }

        friend struct enumerator<builder>;  // to let enumerator access private members

        typedef enumerator<builder> iterator;
        iterator get_iterator_at(uint64_t pos) const { return iterator(this, pos); }
        iterator begin() const { return get_iterator_at(0); }

        void build(compact_vector& cv) {
            cv.m_size = m_size;
            cv.m_width = m_width;
            cv.m_mask = m_mask;
            cv.m_data.swap(m_data);
            builder().swap(*this);
        }

        void swap(builder& other) {
            std::swap(m_size, other.m_size);
            std::swap(m_width, other.m_width);
            std::swap(m_mask, other.m_mask);
            std::swap(m_back, other.m_back);
            std::swap(m_cur_block, other.m_cur_block);
            std::swap(m_cur_shift, other.m_cur_shift);
            m_data.swap(other.m_data);
        }

        uint64_t back() const { return m_back; }
        uint64_t size() const { return m_size; }
        uint64_t width() const { return m_width; }
        VECTOR(uint64_t) const& data() const { return m_data; }

    private:
        uint64_t m_size;
        uint64_t m_width;
        uint64_t m_mask;
        uint64_t m_back;
        uint64_t m_cur_block;
        int64_t m_cur_shift;
        VECTOR(uint64_t) m_data;
    };

    compact_vector() : m_size(0), m_width(0), m_mask(0) {}
    compact_vector(uint64_t size, uint64_t width, uint64_t mask, VECTOR(uint64_t) data)
        : m_size(size), m_width(width), m_mask(mask), m_data(data) {}

    template <typename Iterator>
    void build(Iterator begin, uint64_t n) {
        assert(n > 0);
        uint64_t max = *std::max_element(begin, begin + n);
        uint64_t width = max == 0 ? 1 : std::ceil(std::log2(max + 1));
        build(begin, n, width);
    }

    template <typename Iterator>
    void build(Iterator begin, uint64_t n, uint64_t w) {
        compact_vector::builder builder(begin, n, w);
        builder.build(*this);
    }

    uint64_t operator[](uint64_t i) const {
        assert(i < size());
        uint64_t pos = i * m_width;
        uint64_t block = pos >> 6;
        uint64_t shift = pos & 63;
        return shift + m_width <= 64
                   ? m_data[block] >> shift & m_mask
                   : (m_data[block] >> shift) | (m_data[block + 1] << (64 - shift) & m_mask);
    }

    uint64_t access(uint64_t i) const {
        assert(i < size());
        uint64_t pos = i * m_width;
        const char* ptr = reinterpret_cast<const char*>(m_data.data());
        return (*(reinterpret_cast<uint64_t const*>(ptr + (pos >> 3))) >> (pos & 7)) & m_mask;
    }

    uint64_t back() const { return operator[](size() - 1); }
    uint64_t size() const { return m_size; }
    uint64_t width() const { return m_width; }
    VECTOR(uint64_t) const& data() const { return m_data; }

    typedef enumerator<compact_vector> iterator;
    iterator get_iterator_at(uint64_t pos) const { return iterator(this, pos); }
    iterator begin() const { return get_iterator_at(0); }

    uint64_t num_bytes() const {
        return sizeof(m_size) + sizeof(m_width) + sizeof(m_mask) + essentials::vec_bytes(m_data);
    }

    void swap(compact_vector& other) {
        std::swap(m_size, other.m_size);
        std::swap(m_width, other.m_width);
        std::swap(m_mask, other.m_mask);
        m_data.swap(other.m_data);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visit_impl(visitor, *this);
    }
    template <typename Visitor>
    void visit(Visitor& visitor) {
        visit_impl(visitor, *this);
    }

    template <typename Visitor>
    void visit(const std::string name, Visitor& visitor) const {
        visit_impl(name, visitor, *this);
    }
    template <typename Visitor>
    void visit(const std::string name, Visitor& visitor) {
        visit_impl(name, visitor, *this);
    }

private:
    uint64_t m_size;
    uint64_t m_width;
    uint64_t m_mask;
    VECTOR(uint64_t) m_data;

    template <typename Visitor, typename T>
    static void visit_impl(Visitor& visitor, T&& t) {
        visitor.visit(t.m_size);
        visitor.visit(t.m_width);
        visitor.visit(t.m_mask);
        visitor.visit(t.m_data);
    }
    template <typename Visitor, typename T>
    static void visit_impl(const std::string, Visitor& visitor, T&& t) {
        visitor.dump("\n      bits::compact_vector(");
        visitor.visit("m_size", t.m_size);
        visitor.dump(", ");
        visitor.visit("m_width", t.m_width);
        visitor.dump(", ");
        visitor.visit("m_mask", t.m_mask);
        visitor.dump(",\n    ");
        visitor.visit("m_data", t.m_data);
        visitor.dump(")");
    }
};

}  // namespace bits
