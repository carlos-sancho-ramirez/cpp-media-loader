
#include "block_matrix.hpp"

#include <cmath>

block_matrix::block_matrix()
{
	cell_count_fast_t index;
	for (index = 0; index < CELLS; index++)
	{
		matrix[index] = 0.0f;
	}
}

const block_matrix::cell_index_fast_t block_matrix::zigzag_to_real[] =
{
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,	53, 60, 61, 54, 47, 55, 62, 63
};

void block_matrix::set_at_zigzag(const cell_index_fast_t index, float value)
{
	const cell_index_fast_t position = zigzag_to_real[index];
	matrix[position] = value;
}

block_matrix::cell_index_fast_t block_matrix::get_index(const side_index_fast_t x, const side_index_fast_t y) const
{
	return y * SIDE + x;
}

block_matrix::element_t block_matrix::get(const side_index_fast_t x, const side_index_fast_t y) const
{
	return matrix[get_index(x,y)];
}

void block_matrix::set(const side_index_fast_t x, const side_index_fast_t y, const element_t value)
{
	matrix[get_index(x,y)] = value;
}

block_matrix block_matrix::operator*(const element_t value) const
{
	block_matrix result;

	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		result.matrix[index] = matrix[index] * value;
	}

	return result;
}

block_matrix &block_matrix::operator=(const block_matrix &other)
{
	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		matrix[index] = other.matrix[index];
	}

	return *this;
}

block_matrix &block_matrix::operator+=(const element_t value)
{
	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		matrix[index] += value;
	}

	return *this;
}

block_matrix &block_matrix::operator-=(const element_t value)
{
	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		matrix[index] -= value;
	}

	return *this;
}

block_matrix &block_matrix::operator/=(const element_t value)
{
	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		matrix[index] /= value;
	}

	return *this;
}

block_matrix block_matrix::operator+(const block_matrix &other) const
{
	block_matrix result;

	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		result.matrix[index] = matrix[index] + other.matrix[index];
	}

	return result;
}

block_matrix block_matrix::operator-(const block_matrix &other) const
{
	block_matrix result;

	for (cell_count_fast_t index=0; index < CELLS; index++)
	{
		result.matrix[index] = matrix[index] - other.matrix[index];
	}

	return result;
}

namespace
{
const double PI = 3.141592653589793;

const block_matrix::element_t dct_constant_c0 = sqrt(1.0 / block_matrix::SIDE);
const block_matrix::element_t dct_constant_cn0 = sqrt(2.0 / block_matrix::SIDE);
const block_matrix::element_t dct_multiplying_arg = PI / (2 * block_matrix::SIDE);

void apply_dct_single_element(block_matrix &dest, const block_matrix &orig,
		const block_matrix::side_index_fast_t u, const block_matrix::side_index_fast_t v)
{
	const block_matrix::element_t cu = (u == 0)? dct_constant_c0 : dct_constant_cn0;
	const block_matrix::element_t cv = (v == 0)? dct_constant_c0 : dct_constant_cn0;
	const block_matrix::element_t c = cu * cv;

	block_matrix::element_t result = 0;
	for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
	{
		for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
		{
			const block_matrix::element_t h_cos_arg = (2 * x + 1) * u;
			const block_matrix::element_t v_cos_arg = (2 * y + 1) * v;
			const block_matrix::element_t element = orig.get(x,y) *
					cos(dct_multiplying_arg * h_cos_arg) * cos(dct_multiplying_arg * v_cos_arg);
			result += element;
		}
	}

	result *= c;
	dest.set(u, v, result);
}

void apply_inverse_dct_single_element(block_matrix &dest, const block_matrix &orig,
		const block_matrix::side_index_fast_t u, const block_matrix::side_index_fast_t v)
{
	block_matrix::element_t result = 0;
	for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
	{
		for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
		{
			const block_matrix::element_t cx = (x == 0)? dct_constant_c0 : dct_constant_cn0;
			const block_matrix::element_t cy = (y == 0)? dct_constant_c0 : dct_constant_cn0;
			const block_matrix::element_t c = cx * cy;

			const block_matrix::element_t h_cos_arg = (2 * u + 1) * x;
			const block_matrix::element_t v_cos_arg = (2 * v + 1) * y;
			const block_matrix::element_t element = c * orig.get(x,y) *
					cos(dct_multiplying_arg * h_cos_arg) * cos(dct_multiplying_arg * v_cos_arg);
			result += element;
		}
	}

	//result *= c;
	dest.set(u, v, result);
}
}

block_matrix block_matrix::extract_dct() const
{
	block_matrix result;

	for (block_matrix::side_count_fast_t v = 0; v < block_matrix::SIDE; v++)
	{
		for (block_matrix::side_count_fast_t u = 0; u < block_matrix::SIDE; u++)
		{
			apply_dct_single_element(result, *this, u, v);
		}
	}

	return result;
}

block_matrix block_matrix::extract_inverse_dct() const
{
	block_matrix result;

	for (block_matrix::side_count_fast_t v = 0; v < block_matrix::SIDE; v++)
	{
		for (block_matrix::side_count_fast_t u = 0; u < block_matrix::SIDE; u++)
		{
			apply_inverse_dct_single_element(result, *this, u, v);
		}
	}

	return result;
}

#ifdef PROJECT_DEBUG_BUILD

#include <sstream>
#include <iostream>

std::string block_matrix::dump() const
{
	std::stringstream stream;
	for (block_matrix::side_count_fast_t y = 0; y < block_matrix::SIDE; y++)
	{
		stream << "  [";
		for (block_matrix::side_count_fast_t x = 0; x < block_matrix::SIDE; x++)
		{
			stream << get(x, y) << ",\t";
		}
		stream << "]" << std::endl;
	}

	return stream.str();
}

#endif // PROJECT_DEBUG_BUILD
