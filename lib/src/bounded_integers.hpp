
#ifndef BOUNDED_INTEGERS_HPP_
#define BOUNDED_INTEGERS_HPP_

#include <climits>
#include <stdint.h>

#include "conf.h"
#include "template_utilities.hpp"

#ifdef BOUNDED_INTEGERS_STRICT
#include "integer_ranges.hpp"
#endif // BOUNDED_INTEGERS_STRICT


template<bool sign, bool minInsideSignedChar, bool minInsideSignedShort,
        bool minInsideSignedInt, bool maxInsideSignedChar,
        bool maxInsideUnsignedChar, bool maxInsideSignedShort,
        bool maxInsideUnsignedShort, bool maxInsideSignedInt>
struct bounded_integer_type;

template<>
struct bounded_integer_type<true, true, true, true, true, true, true, true, true>
{
    typedef char type;
    typedef int_fast8_t fast_type;
};

template<bool minInsideSignedChar, bool maxInsideSignedChar, bool maxInsideUnsignedChar>
struct bounded_integer_type<true, minInsideSignedChar, true, true,
        maxInsideSignedChar, maxInsideUnsignedChar, true, true, true>
{
    typedef int16_t type;
    typedef int_fast16_t fast_type;
};

template<bool minInsideSignedChar, bool minInsideSignedShort,
        bool maxInsideSignedChar, bool maxInsideUnsignedChar,
        bool maxInsideSignedShort, bool maxInsideUnsignedShort>
struct bounded_integer_type<true, minInsideSignedChar, minInsideSignedShort,
        true, maxInsideSignedChar, maxInsideUnsignedChar, maxInsideSignedShort,
        maxInsideUnsignedShort, true>
{
    typedef int32_t type;
    typedef int_fast32_t fast_type;
};

template<bool maxInsideSignedChar>
struct bounded_integer_type< false, true, true, true, maxInsideSignedChar,
        true, true, true, true>
{
    typedef unsigned char type;
    typedef uint_fast8_t fast_type;
};

template<bool maxInsideSignedChar,  bool maxInsideUnsignedChar, bool maxInsideSignedShort>
struct bounded_integer_type<false, true, true, true, maxInsideSignedChar,
        maxInsideUnsignedChar, maxInsideSignedShort, true, true>
{
    typedef uint16_t type;
    typedef uint_fast16_t fast_type;
};

template<bool maxInsideSignedChar, bool maxInsideUnsignedChar,
        bool maxInsideSignedShort, bool maxInsideUnsignedShort,
        bool maxInsideSignedInt>
struct bounded_integer_type< false, true, true, true, maxInsideSignedChar,
        maxInsideUnsignedChar, maxInsideSignedShort,
        maxInsideUnsignedShort, maxInsideSignedInt>
{
    typedef uint32_t type;
    typedef uint_fast32_t fast_type;
};


template<int MIN, int MAX>
struct bounded_integer
{
    typedef typename bounded_integer_type<
        MIN < 0,
        MIN >= CHAR_MIN,
        MIN >= SHRT_MIN,
        MIN >= INT_MIN,
        MAX <= CHAR_MAX,
        MAX <= UCHAR_MAX,
        MAX <= SHRT_MAX,
        MAX <= USHRT_MAX,
        MAX <= INT_MAX>::type no_strict_least;

    typedef typename bounded_integer_type<
        MIN < 0,
        MIN >= CHAR_MIN,
        MIN >= SHRT_MIN,
        MIN >= INT_MIN,
        MAX <= CHAR_MAX,
        MAX <= UCHAR_MAX,
        MAX <= SHRT_MAX,
        MAX <= USHRT_MAX,
        MAX <= INT_MAX>::fast_type no_strict_fast;

#ifdef BOUNDED_INTEGERS_STRICT

    typedef static_integer_range<MIN, MAX, no_strict_least> least;
    typedef static_integer_range<MIN, MAX, no_strict_fast> fast;

#else // BOUNDED_INTEGERS_STRICT

    typedef no_strict_least least;
    typedef no_strict_fast fast;

#endif // BOUNDED_INTEGERS_STRICT

    enum
    {
        MIN_VALUE = MIN,
        MAX_VALUE = MAX,
        RANGE = MAX - MIN,
        POSSIBILITIES = RANGE + 1,

        // Minimum amount of bits required to hold all possible steps, but not its value
        NEEDED_BITS = static_math_log<RANGE,2>::value,

        // Mask that can be applied to a value that the MIN_VALUE has been substracted or MIN_VALUE is 0
        RELATIVE_MASK = (1 << NEEDED_BITS) - 1,
    };
};

#endif /* BOUNDED_INTEGERS_HPP_ */
