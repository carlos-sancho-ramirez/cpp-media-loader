/*
 * test_common.hpp
 *
 *  Created on: 27/03/2014
 *      Author: Carlos Sancho Ramirez
 */

#ifndef TEST_COMMON_HPP_
#define TEST_COMMON_HPP_

#include "smart_pointers.hpp"

#include <iostream>

struct test_result
{
	std::string title;
	bool passed;
	std::string log;

	test_result() { }

	test_result(const std::string &title)
	{
		this->title = title;
	}
};

class test_bench_results
{
	unsigned int _total;

public:
	test_bench_results(unsigned int total, const test_result *tests) : _total(total),
			tests(shared_array<const test_result>::make(tests)) { }

	/**
	 * Number of tests in the tests array
	 */
	unsigned int total() const
	{
		return _total;
	}

	/**
	 * Result of each test processed.
	 */
	shared_array<const test_result> tests;

	/**
	 * Total amount of passed test in this bench.
	 */
	unsigned int passed() const;

	/**
	 * Total amount of failed tests.
	 */
	unsigned int failed() const;
};

class test_bench
{
protected:
	test_result test(const std::string &title, void (*function)(std::ostream &)) throw();

public:
	/**
	 * Runs all the tests for this test bench.
	 * This method is a list of calls to the test method.
	 */
	virtual const test_bench_results run() throw() = 0;
};



#endif /* TEST_COMMON_HPP_ */
