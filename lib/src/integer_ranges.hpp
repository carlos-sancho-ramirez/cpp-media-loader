
#ifndef INTEGER_RANGES_HPP_
#define INTEGER_RANGES_HPP_

#include <execinfo.h>
#include <unistd.h>

/**
 * Object thrown whenever a value is going out of range.
 */
struct out_of_range_error
{
	out_of_range_error()
	{
		void *array[10];
		size_t size = backtrace(array, 10);

		backtrace_symbols_fd(array, size, STDERR_FILENO);
	}
};

/**
 * IntegerRange: Class wrapping an integer with a suitable size and ensures the
 * wrapped integer never gets outside the given value.
 *
 * It is intended to keep this class in a way it can be managed in the code as
 * if a normal integer with no restriction was. This way the resulting code
 * should be able to just declare this class in a typedef and switch its
 * definition to a normal integer if looking for performance.
 */
template<int MIN, int MAX, typename TYPE>
struct static_integer_range
{
	static_assert(MIN <= MAX, "Invalid integer range provided MIN must be less or equla to MAX");

    enum
    {
        MIN_VALUE = MIN,
        MAX_VALUE = MAX,
    };

    typedef TYPE type;

private:
    TYPE _value;

    public:
    static_integer_range() : _value(MAX)
    { }

    static_integer_range(const TYPE value) : _value(value)
    {
        if (MIN > value || value > MAX)
        {
            throw out_of_range_error();
        }
    }

    template<typename RANGE_TYPE>
    static_integer_range(const RANGE_TYPE &value) :
            _value(static_cast<const TYPE>(value))
    {
        if (MIN > _value || _value > MAX)
        {
            throw out_of_range_error();
        }
    }

    static_integer_range<MIN,MAX,TYPE> &operator++() // Prefix (++x)
    {
        if (_value >= MAX)
        {
            throw out_of_range_error();
        }

        _value++;
        return *this;
    }

    static_integer_range<MIN,MAX,TYPE> &operator++(int unused) // Postfix (x++)
    {
        return operator++();
    }

    static_integer_range<MIN,MAX,TYPE> &operator--() // Prefix (--x)
    {
        if (_value <= MIN)
        {
            throw out_of_range_error();
        }

        _value--;
        return *this;
    }

    static_integer_range<MIN,MAX,TYPE> &operator--(int unused) // Postfix (x--)
    {
        return operator--();
    }

    static_integer_range<MIN,MAX,TYPE> &operator+=(TYPE value)
    {
        if (MIN > (_value + value) || (_value + value) > MAX)
        {
            throw out_of_range_error();
        }

        _value += value;
        return *this;
    }

    static_integer_range<MIN,MAX,TYPE> &operator-=(TYPE value)
    {
        if (MIN > (_value - value) || (_value - value) > MAX)
        {
            throw out_of_range_error();
        }

        _value -= value;
        return *this;
    }

    static_integer_range<MIN,MAX,TYPE> &operator&=(TYPE value)
    {
        if (MIN > (_value & value) || (_value & value) > MAX)
        {
            throw out_of_range_error();
        }

        _value &= value;
        return *this;
    }

    static_integer_range<MIN,MAX,TYPE> &operator|=(TYPE value)
    {
        if (MIN > (_value | value) || (_value | value) > MAX)
        {
            throw out_of_range_error();
        }

        _value |= value;
        return *this;
    }

    operator TYPE() const
    {
        return _value;
    }
};

#endif /* INTEGER_RANGES_HPP_ */
