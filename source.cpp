#include <iostream>
#include <string>
#include <numeric>
#include <vector>
#include <algorithm>
#include <ranges>

#include "static_vector.hpp"


template<std::size_t Size>
constexpr char test_1()
{
	static_vector<std::string, Size> vec;
	vec.push_back("Test 1");
	vec.push_back("Test 2");

	vec.clear();

	vec.push_back("Test 3");
	vec.push_back("Test 4");

	auto copy = vec;

	copy.pop_back();

	return copy.back().back();
}

constexpr float test_2()
{
	static_vector<float, 10> vec;
	vec.push_back(1.1f);
	vec.push_back(2.2f);
	vec.push_back(3.3f);

	return std::ranges::fold_left_first(vec, std::plus<float>{}).value();
}

constexpr bool test_3()
{
	static_vector<std::string, 10> vec;
	vec.push_back("Test 1");
	vec.push_back("Test 2");
	vec.push_back("Test 3");

	auto cpy = vec;

	return cpy.size() == vec.size() &&
		std::equal(cpy.data(), cpy.data() + cpy.size(), vec.cbegin(), vec.cend());
}

constexpr int test_4()
{
	static_vector<std::vector<int>, 1> vec;

	auto& v = vec.emplace_back(10, 10);

	return std::ranges::fold_left_first(v, std::plus{}).value();
}

constexpr bool test_5()
{
	static_vector<std::string, 10> vec;

	for (int i = 0; i < 10; i++)
	{
		vec.emplace_back("Some string");
	}

	auto new_vec = std::move(vec);

	if (vec.size() != 10)
	{
		return false;
	}

	if (not std::ranges::all_of(vec, std::ranges::empty))
	{
		return false;
	}

	for (const std::string& str : new_vec)
	{
		if (str != "Some string")
		{
			return false;
		}
	}

	vec.clear();

	if (not vec.empty())
	{
		return false;
	}

	return true;
}

constexpr bool test_6()
{
	static_vector<double, 10> vec(5);

	for (double d : vec)
	{
		if (d != 0.0)
		{
			return false;
		}
	}

	if (vec.size() != 5)
	{
		return false;
	}

	return true;
}

constexpr bool test_7()
{
	static_vector<std::string, 10> vec_1;
	static_vector<std::string, 10> vec_2;

	for (const auto _ : std::views::iota(0, 7))
	{
		vec_1.emplace_back("string 1");
	}

	for (const auto _ : std::views::iota(0, 3))
	{
		vec_1.emplace_back("string 2");
	}

	vec_2 = vec_1;

	return std::ranges::equal(vec_1, vec_2);
}

constexpr bool test_8()
{
	static_vector<std::string, 10> vec_1;
	static_vector<std::string, 10> vec_2;

	for (const auto _ : std::views::iota(0, 3))
	{
		vec_1.emplace_back("string 1");
	}

	for (const auto _ : std::views::iota(0, 7))
	{
		vec_1.emplace_back("string 2");
	}

	vec_2 = vec_1;

	return std::ranges::equal(vec_1, vec_2);
}

constexpr bool test_9()
{
	static_vector<std::string, 10> vec_1;
	static_vector<std::string, 10> vec_2;

	vec_2.emplace_back("string 2");

	for (const auto _ : std::views::iota(0, 7))
	{
		vec_1.emplace_back("string 1");
	}

	vec_2 = vec_1;

	return std::ranges::equal(vec_1, vec_2);
}

constexpr bool test_10()
{
	static_vector<std::string, 10> vec_1;
	static_vector<std::string, 10> vec_2;

	vec_1.emplace_back("string 2");

	for (const auto _ : std::views::iota(0, 7))
	{
		vec_2.emplace_back("string 1");
	}

	vec_2 = vec_1;

	return std::ranges::equal(vec_1, vec_2);
}

template<typename Vec>
constexpr void contiguous_range_test()
{
	static_assert(std::ranges::input_range<Vec>);
	static_assert(std::ranges::forward_range<Vec>);
	static_assert(std::ranges::bidirectional_range<Vec>);
	static_assert(std::ranges::random_access_range<Vec>);
	static_assert(std::ranges::contiguous_range<Vec>);
}

int main()
{
	{
		using trivial_vec = static_vector<float, 100>;

		contiguous_range_test<trivial_vec>();

		static_assert(not std::is_trivially_constructible_v<trivial_vec>);
		static_assert(std::is_trivially_copy_constructible_v<trivial_vec>);
		static_assert(std::is_trivially_move_constructible_v<trivial_vec>);
		static_assert(std::is_trivially_copy_assignable_v<trivial_vec>);
		static_assert(std::is_trivially_move_assignable_v<trivial_vec>);
		static_assert(std::is_trivially_destructible_v<trivial_vec>);
	}
	{
		using vec = static_vector<std::string, 100>;

		contiguous_range_test<vec>();

		static_assert(not std::is_trivially_constructible_v<vec>);
		static_assert(not std::is_trivially_copy_constructible_v<vec>);
		static_assert(not std::is_trivially_move_constructible_v<vec>);
		static_assert(not std::is_trivially_copy_assignable_v<vec>);
		static_assert(not std::is_trivially_move_assignable_v<vec>);
		static_assert(not std::is_trivially_destructible_v<vec>);
	}
	{
		static_assert(test_1<2>() == '3');
		static_assert(test_2() == (1.1f + 2.2f + 3.3f));
		static_assert(test_3() == true);
		static_assert(test_4() == 100);
		static_assert(test_5() == true);
		static_assert(test_6() == true);
		static_assert(test_7() == true);
		static_assert(test_8() == true);
		static_assert(test_9() == true);
		//static_assert(test_10() == true);
	}
}
