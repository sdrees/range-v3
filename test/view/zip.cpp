// Range v3 library
//
//  Copyright Eric Niebler 2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <range/v3/core.hpp>
#include <range/v3/view/zip.hpp>
#include <range/v3/view/zip_with.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/move.hpp>
#include <range/v3/view/stride.hpp>
#include <range/v3/view/bounded.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/move.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/utility/iterator.hpp>
#include "../simple_test.hpp"
#include "../test_utils.hpp"

struct MoveOnlyString
{
    char const *sz_;

    MoveOnlyString(char const *sz = "")
      : sz_(sz)
    {}
    MoveOnlyString(MoveOnlyString &&that)
      : sz_(that.sz_)
    {
        that.sz_ = "";
    }
    MoveOnlyString(MoveOnlyString const &) = delete;
    MoveOnlyString &operator=(MoveOnlyString &&that)
    {
        sz_ = that.sz_;
        that.sz_ = "";
        return *this;
    }
    MoveOnlyString &operator=(MoveOnlyString const &) = delete;
    bool operator==(MoveOnlyString const &that) const
    {
        return 0 == std::strcmp(sz_, that.sz_);
    }
    bool operator!=(MoveOnlyString const &that) const
    {
        return !(*this == that);
    }
    friend std::ostream & operator<< (std::ostream &sout, MoveOnlyString const &str)
    {
        return sout << '"' << str.sz_ << '"';
    }
};

template<typename T>
struct undef_t;

int main()
{
    using namespace ranges;

    std::vector<int> vi{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<std::string> const vs{"hello", "goodbye", "hello", "goodbye"};

    // All ranges
    {
        std::stringstream str{"john paul george ringo"};
        using V = std::tuple<int, std::string, std::string>;
        auto && rng = view::zip(vi, vs, istream<std::string>(str) | view::bounded);
        using Rng = decltype(rng);
        ::models<concepts::BoundedRange>(rng);
        ::models_not<concepts::SizedRange>(rng);
        CONCEPT_ASSERT(Same<
            range_value_t<Rng>,
            std::tuple<int, std::string, std::string>>());
        CONCEPT_ASSERT(Same<
            range_reference_t<Rng>,
            common_tuple<int &, std::string const &, std::string const &>>());
        CONCEPT_ASSERT(Same<
            range_rvalue_reference_t<Rng>,
            std::tuple<int &&, std::string const &&, std::string const &&>>());
        CONCEPT_ASSERT(Convertible<range_value_t<Rng> &&,
            range_rvalue_reference_t<Rng>>());
        ::models<concepts::InputIterator>(begin(rng));
        ::models_not<concepts::ForwardIterator>(begin(rng));
        std::vector<V> expected(begin(rng), end(rng));
        ::check_equal(expected, {V{0, "hello", "john"},
                                 V{1, "goodbye", "paul"},
                                 V{2, "hello", "george"},
                                 V{3, "goodbye", "ringo"}});
    }

    // Mixed ranges and bounded ranges
    {
        std::stringstream str{"john paul george ringo"};
        using V = std::tuple<int, std::string, std::string>;
        auto && rng = view::zip(vi, vs, istream<std::string>(str));
        ::models<concepts::Range>(rng);
        ::models_not<concepts::SizedRange>(rng);
        ::models_not<concepts::BoundedRange>(rng);
        ::models<concepts::InputIterator>(begin(rng));
        ::models_not<concepts::ForwardIterator>(begin(rng));
        std::vector<V> expected;
        copy(rng, ranges::back_inserter(expected));
        ::check_equal(expected, {V{0, "hello", "john"},
                                 V{1, "goodbye", "paul"},
                                 V{2, "hello", "george"},
                                 V{3, "goodbye", "ringo"}});
    }

    auto rnd_rng = view::zip(vi, vs);
    using Ref = range_reference_t<decltype(rnd_rng)>;
    static_assert(std::is_same<Ref, common_pair<int &,std::string const &>>::value, "");
    ::models<concepts::BoundedRange>(rnd_rng);
    ::models<concepts::SizedRange>(rnd_rng);
    ::models<concepts::RandomAccessIterator>(begin(rnd_rng));
    auto tmp = cbegin(rnd_rng) + 3;
    CHECK(std::get<0>(*tmp) == 3);
    CHECK(std::get<1>(*tmp) == "goodbye");

    CHECK((rnd_rng.end() - rnd_rng.begin()) == 4);
    CHECK((rnd_rng.begin() - rnd_rng.end()) == -4);
    CHECK(rnd_rng.size() == 4u);

    // zip_with
    {
        std::vector<std::string> v0{"a","b","c"};
        std::vector<std::string> v1{"x","y","z"};

        auto rng = view::zip_with(std::plus<std::string>{}, v0, v1);
        std::vector<std::string> expected;
        copy(rng, ranges::back_inserter(expected));
        ::check_equal(expected, {"ax","by","cz"});
    }

    // zip_with
    {
        std::vector<std::string> v0{"a","b","c"};
        std::vector<std::string> v1{"x","y","z"};

        auto rng = view::zip_with(std::plus<std::string>{}, v0, v1);
        std::vector<std::string> expected;
        copy(rng, ranges::back_inserter(expected));
        ::check_equal(expected, {"ax","by","cz"});
    }

    // Move from a zip view
    {
        auto v0 = to_<std::vector<MoveOnlyString>>({"a","b","c"});
        auto v1 = to_<std::vector<MoveOnlyString>>({"x","y","z"});

        auto rng = view::zip(v0, v1);
        ::models<concepts::RandomAccessIterable>(rng);
        std::vector<std::pair<MoveOnlyString, MoveOnlyString>> expected;
        move(rng, ranges::back_inserter(expected));
        ::check_equal(expected | view::keys, {"a","b","c"});
        ::check_equal(expected | view::values, {"x","y","z"});
        ::check_equal(v0, {"","",""});
        ::check_equal(v1, {"","",""});

        move(expected, rng.begin());
        ::check_equal(expected | view::keys, {"","",""});
        ::check_equal(expected | view::values, {"","",""});
        ::check_equal(v0, {"a","b","c"});
        ::check_equal(v1, {"x","y","z"});

        std::vector<MoveOnlyString> res;
        using R = decltype(rng);
        auto proj =
            [](range_reference_t<R> p) -> MoveOnlyString& {return p.first;};
        auto rng2 = rng | view::transform(proj);
        move(rng2, ranges::back_inserter(res));
        ::check_equal(res, {"a","b","c"});
        ::check_equal(v0, {"","",""});
        ::check_equal(v1, {"x","y","z"});
        using R2 = decltype(rng2);
        CONCEPT_ASSERT(Same<range_value_t<R2>, MoveOnlyString>());
        CONCEPT_ASSERT(Same<range_reference_t<R2>, MoveOnlyString &>());
        CONCEPT_ASSERT(Same<range_rvalue_reference_t<R2>, MoveOnlyString &&>());
    }

    {
        auto const v = to_<std::vector<MoveOnlyString>>({"a","b","c"});
        auto rng = view::zip(v, v);
        using Rng = decltype(rng);
        using I = range_iterator_t<Rng>;
        CONCEPT_ASSERT(Readable<I>());
        CONCEPT_ASSERT(Same<
            range_value_t<Rng>,
            std::pair<MoveOnlyString, MoveOnlyString>>());
        CONCEPT_ASSERT(Same<
            range_reference_t<Rng>,
            common_pair<MoveOnlyString const &, MoveOnlyString const &>>());
        CONCEPT_ASSERT(Same<
            range_rvalue_reference_t<Rng>,
            std::pair<MoveOnlyString const &&, MoveOnlyString const &&>>());
        CONCEPT_ASSERT(Same<
            range_common_reference_t<Rng>,
            common_pair<MoveOnlyString const &, MoveOnlyString const &>>());
    }

    {
        std::vector<int> v{1,2,3,4};
        auto moved = v | view::move;
        using Moved = decltype(moved);
        CONCEPT_ASSERT(Same<range_reference_t<Moved>, int &&>());
        auto zipped = view::zip(moved);
        using Zipped = decltype(zipped);
        CONCEPT_ASSERT(Same<range_reference_t<Zipped>, common_tuple<int &&> >());
    }

    // This is actually a test of the logic of range_adaptor. Since the stride view
    // does not redefine the current member function, the base range's indirect_move
    // function gets picked up automatically.
    {
        auto rng0 = view::zip(vi, vs);
        auto rng1 = view::stride(rng0, 2);
        CONCEPT_ASSERT(Same<range_rvalue_reference_t<decltype(rng1)>, range_rvalue_reference_t<decltype(rng0)>>());
        CONCEPT_ASSERT(Same<range_value_t<decltype(rng1)>, range_value_t<decltype(rng0)>>());
    }

    return test_result();
}
