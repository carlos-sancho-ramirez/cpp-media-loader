/*
 * test_common.cpp
 *
 *  Created on: 27/03/2014
 *      Author: Carlos Sancho Ramirez
 */

#include "test_common.hpp"

#include <sstream>

unsigned int test_bench_results::passed() const
{
	unsigned int result = 0;
	for (unsigned int index = 0; index < _total; index++)
	{
		if (tests[index].passed)
		{
			++result;
		}
	}

	return result;
}

unsigned int test_bench_results::failed() const
{
	return _total - passed();
}

test_result test_bench::test(const std::string &title, void (*function)(std::ostream &)) throw()
{
	test_result result(title);
	std::stringstream test_stream;
	try
	{
		function(test_stream);
		result.passed = true;
	}
	catch (std::exception &)
	{
		result.passed = false;
		result.log = test_stream.str();
	}
	catch (...)
	{
		result.passed = false;
		result.log = test_stream.str();
	}

	return result;
}
