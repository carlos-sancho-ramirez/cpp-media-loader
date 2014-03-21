
#ifndef TEMPLATE_UTILITIES_HPP_
#define TEMPLATE_UTILITIES_HPP_

/**
 * Allows calculating powers in compile time.
 *
 * Example: 2^5, which is 32, can be calculated in compile time by retrieving
 * the value in MathPower<2,5>::value
 */
template<int base, unsigned int exponent>
struct static_math_power
{
    enum
    {
        value = base * static_math_power<base, exponent - 1>::value
    };
};

template<int base>
struct static_math_power<base,0>
{
    enum
    {
        value = 1
    };
};


/**
 * MathLog retrieves the floor integer representation of the result of
 * calculating the logarithm of a given base.
 *
 * Example:
 *  * the logarithm of 5 for base 2 is 2.3219.
 *  * the floor integer is in this case 2 (I mean by floor removing the decimal
 *    cyphers).
 *  * Then, in compile time this value can be extracted by retrieving the value
 *    at MathLog<5,2>::value.
 */
template<int num, int base, unsigned int count>
struct static_math_logarithm_internal
{
    enum
    {
        value = static_math_logarithm_internal<num / base, base, count + 1>::value
    };
};

template<int base, unsigned int count>
struct static_math_logarithm_internal<0, base, count>
{
    enum
    {
        value = count
    };
};

template<int num, int base>
struct static_math_log
{
    enum
    {
        value = static_math_logarithm_internal<num,base,0>::value
    };
};

#endif /* TEMPLATE_UTILITIES_HPP_ */
