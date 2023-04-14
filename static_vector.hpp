#pragma once

#include <array>
#include <concepts>
#include <format>
#include <type_traits>

namespace detail
{
	template<std::input_iterator InputIt, typename Size, std::forward_iterator ForwardIt>
	constexpr ForwardIt constexpr_uninitialized_copy_n(InputIt first, Size count, ForwardIt d_first)
	{
		if (not std::is_constant_evaluated())
		{
			std::uninitialized_copy_n(std::move(first),
				std::move(count), std::move(d_first));
			return;
		}

		using T = std::iter_value_t<ForwardIt>;
		ForwardIt current = d_first;
		try {
			for (; count > 0; ++first, (void) ++current, --count) {
				std::construct_at(std::to_address(current), *first);
			}
		}
		catch (...) {
			for (; d_first != current; ++d_first) {
				std::destroy_at(std::to_address(d_first));
			}
			throw;
		}
		return current;
	}
}

template<typename T, std::size_t Capacity>
class static_vector
{
public:
	constexpr static_vector() noexcept = default;

	constexpr static_vector(const static_vector&) noexcept
		requires std::is_trivially_copy_constructible_v<T>
	= default;

	constexpr static_vector(static_vector&&) noexcept
		requires std::is_trivially_move_constructible_v<T>
	= default;

	constexpr static_vector& operator=(const static_vector&) noexcept
		requires std::is_trivially_copy_assignable_v<T>
	= default;

	constexpr static_vector& operator=(static_vector&&) noexcept
		requires std::is_trivially_move_assignable_v<T>
	= default;

	constexpr static_vector(const static_vector& other)
		noexcept (std::is_nothrow_copy_constructible_v<T>)
		requires (not std::is_trivially_copy_constructible_v<T>)
		:size_{ other.size() }
	{
		detail::constexpr_uninitialized_copy_n(other.cbegin(), other.size(), data_);
	}

	constexpr T& operator[](std::size_t offset) noexcept
	{
		return data_[offset];
	}

	constexpr const T& operator[](std::size_t offset) const noexcept
	{
		return data_[offset];
	}

	constexpr T& at(std::size_t offset)
	{
		if (offset >= size_)
		{
			throw std::out_of_range{ std::format("Index {} is out of "
				"the range range of the vector. Range is [0, {})!",
				offset, size_) };
		}

		return data_[offset];
	}

	constexpr const T& at(std::size_t offset) const
	{
		if (offset >= size_)
		{
			throw std::out_of_range{ std::format("Index {} is out of "
				"the range range of the vector. Range is [0, {})!",
				offset, size_) };
		}

		return data_[offset];
	}

	constexpr T* data() noexcept
	{
		return data_;
	}

	constexpr const T* data() const noexcept
	{
		return data_;
	}

	constexpr T& back() noexcept
	{
		return data_[size_ - 1];
	}

	constexpr const T& back() const noexcept
	{
		return data_[size_ - 1];
	}

	constexpr T& front() noexcept
	{
		return &data_[0];
	}

	constexpr const T& front() const noexcept
	{
		return &data_[0];
	}

	constexpr void push_back(const T& value)
	{
		if (size_ == Capacity)
		{
			throw std::length_error{ std::format("Static vector push_back call would "
				"exceed the vector's capacity of {}", Capacity) };
		}

		std::construct_at(data_ + size_, value);
		++size_;
	}

	constexpr void push_back(T&& value)
	{
		if (size_ == Capacity)
		{
			throw std::length_error{ std::format("Static vector push_back call would "
				"exceed the vector's capacity of {}", Capacity) };
		}

		std::construct_at(data_ + size_, std::move(value));
		++size_;
	}

	template<typename ... Args>
	requires std::is_constructible_v<T, Args...>
	constexpr T& emplace_back(Args&& ... args)
	{
		if (size_ == Capacity)
		{
			throw std::length_error{ std::format("Static vector emplace_back call would "
				"exceed the vector's capacity of {}", Capacity) };
		}

		std::construct_at(data_ + size_, std::forward<Args>(args)...);
		++size_;

		return back();
	}

	constexpr std::size_t size() const noexcept
	{
		return size_;
	}

	constexpr std::size_t capacity() const noexcept
	{
		return Capacity;
	}

	constexpr bool empty() const noexcept
	{
		return size_ == 0;
	}

	constexpr void clear()
		noexcept(std::is_nothrow_destructible_v<T>)
	{
		std::destroy_n(data_, size_);

		if (std::is_constant_evaluated())
		{
			dummy_ = std::byte{};
		}

		size_ = 0;
	}

	constexpr ~static_vector()
	noexcept(std::is_nothrow_destructible_v<T>)
	requires(not std::is_trivially_destructible_v<T>)
	{
		std::destroy_n(data_, size_);
	}

	constexpr ~static_vector() noexcept
	requires(std::is_trivially_destructible_v<T>)
		= default;

	constexpr T* begin() noexcept
	{
		return data_;
	}

	constexpr T* end() noexcept
	{
		return &data_[size_];
	}

	constexpr const T* begin() const noexcept
	{
		return data_;
	}

	constexpr const T* end() const noexcept
	{
		return &data_[size_];
	}

	constexpr const T* cbegin() const noexcept
	{
		return data_;
	}

	constexpr const T* cend() const noexcept
	{
		return &data_[size_];
	}

private:
	union {
		std::byte dummy_{};
		T data_[Capacity];
	};
	std::size_t size_ = 0;
};