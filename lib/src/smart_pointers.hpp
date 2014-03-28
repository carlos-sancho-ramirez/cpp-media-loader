
#ifndef SMART_POINTERS_HPP_
#define SMART_POINTERS_HPP_

/**
 * Shares ownership of a data on the heap acquired by calling new[].
 * This class will call delete[] on the pointer when no other shared_array will point to it.
 */
template<class TYPE>
class shared_array
{
	unsigned int *shared_counter;
	TYPE *data;

	void decrease_count()
	{
		if (shared_counter != 0)
		{
			if (--(*shared_counter) == 0)
			{
				delete shared_counter;
				delete[] data;
			}
		}
	}

public:
	shared_array() : shared_counter(0), data(0) { }
	shared_array(const shared_array<TYPE> &other) : shared_counter(other.shared_counter), data(data)
	{
		if (shared_counter != 0)
		{
			++(*shared_counter);
		}
	}

	~shared_array()
	{
		decrease_count();
	}

	shared_array<TYPE> &operator=(const shared_array<TYPE> &other)
	{
		decrease_count();

		shared_counter = other.shared_counter;
		data = other.data;

		if (shared_counter != 0)
		{
			++(*shared_counter);
		}

		return *this;
	}

	TYPE * const operator->() const
	{
		return data;
	}

	TYPE *operator->()
	{
		return data;
	}

	TYPE &operator[](const unsigned int index) const
	{
		return data[index];
	}

	TYPE &operator[](const unsigned int index)
	{
		return data[index];
	}

	operator const TYPE *() const
	{
		return data;
	}

	TYPE * const get() const
	{
		return data;
	}

	static shared_array<TYPE> make(TYPE *data)
	{
		unsigned int *counter = new unsigned int(1);

		shared_array<TYPE> result;
		result.data = data;
		result.shared_counter = counter;

		return result;
	}
};



#endif /* SMART_POINTERS_HPP_ */
