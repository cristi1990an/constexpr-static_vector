#pragma once

#include <array>
#include <concepts>
#include <format>
#include <type_traits>

namespace detail
{
	template<std::size_t Max>
	struct required_bits
	{
		static constexpr std::size_t value = Max <= 0xff ? 8 :
											 Max <= 0xffff ? 16 :
											 Max <= 0xffffffff ? 32 : 64;
	};

	template<std::size_t Bits> struct select_smallest_value_type;
	template<> struct select_smallest_value_type <8> { using type = uint8_t; };
	template<> struct select_smallest_value_type<16> { using type = uint16_t; };
	template<> struct select_smallest_value_type<32> { using type = uint32_t; };
	template<> struct select_smallest_value_type<64> { using type = uint64_t; };

	template<std::size_t Max>
	struct smallest_size_type : select_smallest_value_type<required_bits<Max>::value> {};

	template<std::size_t Max>
	using smallest_size_type_t = typename smallest_size_type<Max>::type;

	template<std::input_iterator InputIt, std::integral Size, std::forward_iterator ForwardIt>
	constexpr ForwardIt constexpr_uninitialized_copy_n(InputIt first, Size count, ForwardIt d_first)
	{
		if (not std::is_constant_evaluated())
		{
			return std::uninitialized_copy_n(std::move(first),
				std::move(count), std::move(d_first));
		}

		using T = std::iter_value_t<ForwardIt>;
		ForwardIt current = d_first;
		try
		{
			for (; count > 0; ++first, (void) ++current, --count)
			{
				std::construct_at(std::to_address(current), *first);
			}
		}
		catch (...)
		{
			std::destroy(d_first, current);
			throw;
		}
		return current;
	}

	template<std::input_iterator InputIt, std::integral Size, std::forward_iterator ForwardIt>
	constexpr std::pair<InputIt, ForwardIt>
		constexpr_uninitialized_move_n(InputIt first, Size count, ForwardIt d_first)
	{
		if (not std::is_constant_evaluated())
		{
			return std::uninitialized_move_n(std::move(first),
				std::move(count), std::move(d_first));
		}

		using Value = std::iter_value_t<ForwardIt>;
		ForwardIt current = d_first;
		try
		{
			for (; count > 0; ++first, (void) ++current, --count)
			{
				std::construct_at(std::to_address(current), std::move(*first));
			}
		}
		catch (...)
		{
			std::destroy(d_first, current);
			throw;
		}
		return { first, current };
	}

	template<std::forward_iterator ForwardIt, std::integral Size>
	constexpr ForwardIt constexpr_uninitialized_value_construct_n(ForwardIt first, Size n)
	{
		if (not std::is_constant_evaluated())
		{
			return std::uninitialized_value_construct_n(std::move(first), std::move(n));
		}

		using T = std::iter_value_t<ForwardIt>;

		ForwardIt current = first;
		try
		{
			for (; n > 0; (void) ++current, --n)
			{
				std::construct_at(std::to_address(current), T{});
			}
			return current;
		}
		catch (...)
		{
			std::destroy(first, current);
			throw;
		}
	}

	template<std::forward_iterator ForwardIt, std::integral Size, typename T>
	constexpr ForwardIt constexpr_uninitialized_fill_n(ForwardIt first, Size count, const T& value)
	{
		if (not std::is_constant_evaluated())
		{
			return std::uninitialized_fill_n(std::move(first),
				std::move(count), value);;
		}

		using V = std::iter_value_t<ForwardIt>;
		ForwardIt current = first;
		try 
		{
			for (; count > 0; ++current, (void) --count)
			{
				std::construct_at(std::to_address(current), value);
			}
			return current;
		}
		catch (...) 
		{
			std::destroy(first, current);
			throw;
		}
	}
}

template<typename T, std::size_t Capacity>
class static_vector
{
private:
	using real_size_t = detail::smallest_size_type_t<Capacity>;

public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<T*>;
	using const_reverse_iterator =
		std::reverse_iterator<const T*>;

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
		:size_{ other.size_ }
	{
		detail::constexpr_uninitialized_copy_n(other.cbegin(), other.size(), data());
	}
		
	template<size_type OtherCapacity>
	explicit(false) constexpr static_vector(const static_vector<T, OtherCapacity>& other)
		noexcept (std::is_nothrow_copy_constructible_v<T>)
		requires (OtherCapacity < Capacity)
		: size_{ static_cast<real_size_t>(other.size()) }
	{
		detail::constexpr_uninitialized_copy_n(other.cbegin(), other.size(), data());
	}

	template<size_type OtherCapacity>
	explicit(false) constexpr static_vector(const static_vector<T, OtherCapacity>& other)
		requires (OtherCapacity > Capacity)
		: size_{ static_cast<real_size_t>(other.size()) }
	{
		if (other.size() > capacity()) [[unlikely]]
		{
			throw std::length_error(std::format("Attempting to construct static_vector with a "
				"max capacity of {} from a static_vector of {} elements", capacity(), other.size()));
		}

		detail::constexpr_uninitialized_copy_n(other.cbegin(), other.size(), data());
	}

	constexpr static_vector(static_vector&& other)
		noexcept (std::is_nothrow_move_constructible_v<T>
			|| std::is_nothrow_copy_constructible_v<T>)
		requires (not std::is_trivially_move_constructible_v<T>)
		: size_{ other.size_ }
	{
		if constexpr (not std::is_nothrow_move_constructible_v<T>)
		{
			detail::constexpr_uninitialized_copy_n(other.cbegin(), other.size(), data_);
			other.clear();
			return;
		}
		detail::constexpr_uninitialized_move_n(other.begin(), other.size(), data_);
	}

	explicit(false) constexpr static_vector(std::initializer_list<T> init)
		: size_{ static_cast<real_size_t>(init.size()) }
	{
		if (init.size() > Capacity) [[unlikely]]
		{
			throw std::length_error(std::format("Attempting to construct static_vector with a "
				"max capacity of {} from an initializer_list of {} elements", capacity(), init.size()));
		}

		detail::constexpr_uninitialized_copy_n(init.begin(), init.size(), data());
	}

	template<std::input_iterator InputIt>
	constexpr static_vector(InputIt first, InputIt last)
	{
		if constexpr (std::forward_iterator<InputIt>)
		{
			size_ = std::distance(first, last);
			if (size_ > Capacity) [[unlikely]]
			{
				throw std::length_error(std::format("Attempting to construct static_vector with a "
					"max capacity of {} from a range of {} elements", Capacity, size_));
			}

			detail::constexpr_uninitialized_copy_n(first, size, data());
		}
		else
		{
			while (first != last)
			{
				push_back(*first);
				++first;
			}
		}
	}

	template<std::ranges::input_range Range>
	constexpr static_vector(std::from_range_t, Range&& range)
	{
		if constexpr (std::ranges::sized_range<Range>)
		{
			size_ = std::ranges::size(range);
			if (size_ > Capacity) [[unlikely]]
			{
				throw std::length_error(std::format("Attempting to construct static_vector with a "
					"max capacity of {} from a range of {} elements", Capacity, size_));
			}

			detail::constexpr_uninitialized_copy_n(std::ranges::begin(range), size, data());
		}
		else
		{
			std::ranges::copy(std::forward<Range>(range), std::back_inserter(*this));
		}
	}

	constexpr explicit static_vector(size_type count)
		noexcept (std::is_nothrow_default_constructible_v<T>)
		requires (std::is_default_constructible_v<T>)
		: size_{ static_cast<real_size_t>(count) }
	{
		detail::constexpr_uninitialized_value_construct_n(data(), count);
	}

	constexpr explicit static_vector(size_type count, const T& value)
		noexcept (std::is_nothrow_copy_constructible_v<T>)
		: size_{ static_cast<real_size_t>(count) }
	{
		detail::constexpr_uninitialized_fill_n(data(), count, value);
	}

	constexpr static_vector& operator=(const static_vector& other)
	noexcept(std::is_nothrow_copy_assignable_v<T>)
		requires(not std::is_trivially_copy_assignable_v<T>)
	{
		if (this == std::addressof(other)) [[unlikely]]
		{
			return *this;
		}

		const size_type min_size = std::min(size(), other.size());
		const pointer new_end = data() + min_size;

		std::ranges::copy_n(other.cbegin(), min_size, begin());
		detail::constexpr_uninitialized_copy_n(other.cbegin() + min_size, other.size() - min_size, new_end);
		std::destroy_n(new_end, size() - min_size);

		size_ = static_cast<real_size_t>(other.size());

		return *this;
	}

	constexpr static_vector& operator=(static_vector&& other)
		noexcept(std::is_nothrow_move_assignable_v<T>)
		requires(not std::is_trivially_move_assignable_v<T>)
	{
		if (this == std::addressof(other)) [[unlikely]]
		{
			return *this;
		}

		const size_type min_size = std::min(size(), other.size());
		const pointer new_end = data() + min_size;

		std::ranges::copy_n(std::make_move_iterator(other.begin()), min_size, begin());
		detail::constexpr_uninitialized_move_n(other.begin() + min_size, other.size() - min_size, new_end);
		std::destroy_n(new_end, size() - min_size);

		size_ = static_cast<real_size_t>(other.size());

		other.clear();

		return *this;
	}

	constexpr void swap(static_vector& other)
		noexcept(std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>)
	{
		const size_type min_size = std::min(size(), other.size());

		std::ranges::swap_ranges(begin(), begin() + min_size,
			other.begin(), other.begin() + min_size);

		detail::constexpr_uninitialized_move_n(other.begin() + min_size, other.size() - min_size, data() + size());
		detail::constexpr_uninitialized_move_n(begin() + min_size, size() - min_size, other.data() + other.size());
		std::destroy_n(other.data() + min_size, other.size() - min_size);
		std::destroy_n(data() + min_size, size() - min_size);

		std::swap(size_, other.size_);
	}

	constexpr void assign(std::initializer_list<T> init)
	{
		if (init.size() > capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector assign call with initializer_list "
				"of size {} being passed would exceed the vector's capacity of {}",
				init.size(), capacity()) };
		}

		const size_type min_size = std::min(size(), init.size());

		std::ranges::copy_n(init.begin(), min_size, begin());
		detail::constexpr_uninitialized_copy_n(
			init.begin() + min_size, init.size() - min_size, data() + size());
		std::destroy_n(data() + size(), size() - min_size);

		size_ = static_cast<real_size_t>(init.size());
	}

	constexpr void assign(size_type count, const T& value)
	{
		if (count > capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector assign call for {} elements "
				"would exceed the vector's capacity of {}",
				count, capacity()) };
		}

		const size_type min_size = std::min(size(), count);

		std::ranges::fill_n(begin(), min_size, value);
		detail::constexpr_uninitialized_fill_n(data() + size(), count - min_size, value);
		std::destroy_n(data() + size(), size() - min_size);

		size_ = static_cast<real_size_t>(count);
	}

	template<typename Range>
	constexpr void assign_range(Range&& range)
		requires (std::constructible_from<T, std::ranges::range_value_t<Range>>)
	{
		if constexpr (std::ranges::sized_range<Range> || std::ranges::forward_range<Range>)
		{
			const size_type rsize = [&]()
			{
				if constexpr (std::ranges::sized_range<Range>)
				{
					return std::ranges::size(range);
				}
				else
				{
					return std::ranges::distance(range);
				}
			}();

			if (rsize > capacity()) [[unlikely]]
			{
				throw std::length_error{ std::format("Static vector assign call for range of {} "
					"elements would exceed the vector's capacity of {}",
					rsize, capacity()) };
			}

			const size_type min_size = std::min(size(), rsize);

			auto [it, _] = std::ranges::copy_n(std::ranges::begin(range), min_size, begin());
			detail::constexpr_uninitialized_copy_n(std::move(it), rsize - min_size, data() + size());
			std::destroy_n(data() + size(), size() - min_size);

			size_ = static_cast<real_size_t>(rsize);
		}
		else
		{
			clear();
			std::ranges::copy(std::forward<Range>(range), std::back_inserter(*this));
		}
	}

	template<std::input_iterator InputIt>
	constexpr void assign(InputIt first, InputIt last)
		requires (std::constructible_from<T, std::iter_value_t<InputIt>>)
	{
		assign_range(std::ranges::subrange(std::move(first), std::move(last)));
	}

	constexpr reference operator[](size_type offset) noexcept
	{
		return data_[offset];
	}

	constexpr const_reference operator[](size_type offset) const noexcept
	{
		return data_[offset];
	}

	constexpr reference at(size_type offset)
	{
		if (offset >= size()) [[unlikely]]
		{
			throw std::out_of_range{ std::format("Index {} is out of "
				"the range of the vector. Range is [0, {})!",
				offset, size()) };
		}

		return data_[offset];
	}

	constexpr const_reference at(size_type offset) const
	{
		if (offset >= size()) [[unlikely]]
		{
			throw std::out_of_range{ std::format("Index {} is out of "
				"the range of the vector. Range is [0, {})!",
				offset, size()) };
		}

		return data_[offset];
	}

	constexpr pointer data() noexcept
	{
		return data_;
	}

	constexpr const_pointer data() const noexcept
	{
		return data_;
	}

	constexpr reference back() noexcept
	{
		return data_[size_ - 1];
	}

	constexpr const_reference back() const noexcept
	{
		return data_[size_ - 1];
	}

	constexpr reference front() noexcept
	{
		return &data_[0];
	}

	constexpr const_reference front() const noexcept
	{
		return &data_[0];
	}

	constexpr iterator insert(const_iterator pos, const T& value)
	{
		if (size() == capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector insert call would "
				"exceed the vector's capacity of {}", Capacity) };
		}

		if (pos == cend()) [[unlikely]]
		{
			push_back(value);
			return begin() + std::distance(cbegin(), pos);
		}

		std::construct_at(data() + size(), std::move(back()));

		iterator it = begin() + std::distance(cbegin(), pos);
		std::ranges::move_backward(it, end() - 1, it + 1);

		try
		{
			*it = value;
		}
		catch (...)
		{
			std::ranges::move(it + 1, end() + 1, it);
			throw;
		}

		++size_;

		return it;
	}

	constexpr iterator insert(const_iterator pos, T&& value)
	{
		if (size() == capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector insert call would "
				"exceed the vector's capacity of {}", capacity()) };
		}

		if (pos == cend()) [[unlikely]]
		{
			push_back(std::move(value));
			return begin() + std::distance(cbegin(), pos);
		}

		std::construct_at(data() + size(), std::move(back()));

		iterator it = begin() + std::distance(cbegin(), pos);
		std::ranges::move_backward(it, end() - 1, end());

		try
		{
			*it = std::move(value);
		}
		catch (...)
		{
			std::ranges::move(it + 1, end() + 1, it);
			throw;
		}

		++size_;

		return it;
	}

	template<typename ... Args>
	constexpr iterator emplace(const_iterator pos, Args&& ... args)
	{
		if (size() == capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector emplace call would "
				"exceed the vector's capacity of {}", capacity()) };
		}

		if (pos == cend()) [[unlikely]]
		{
			emplace_back(std::forward<Args>(args)...);
			return begin() + std::distance(cbegin(), pos);
		}

		std::construct_at(data() + size(), std::move(back()));

		iterator it = begin() + std::distance(cbegin(), pos);
		std::ranges::move_backward(it, end() - 1, end());

		try
		{
			*it = T(std::forward<Args>(args)...);
		}
		catch (...)
		{
			std::ranges::move(it + 1, end() + 1, it);
			throw;
		}

		++size_;

		return it;
	}

	constexpr iterator insert(const_iterator pos, size_type count, const T& value)
	{
		if (size() + count > capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector insert for {} elements "
				"when the vector already stores {} elements would exceed the vector's "
				"capacity of {}", count, size(), capacity()) };
		}

		if (pos == cend()) [[unlikely]]
		{
			detail::constexpr_uninitialized_fill_n(data() + size(), count, value);
			size_ += static_cast<real_size_t>(count);
			return begin() + std::distance(cbegin(), pos);
		}

		iterator it = begin() + std::distance(cbegin(), pos);
		detail::constexpr_uninitialized_move_n(end() - count, count, data() + size());
		std::ranges::move(std::make_reverse_iterator(end() - count),
			std::make_reverse_iterator(it),
			std::make_reverse_iterator(end()));
		std::ranges::fill_n(it, count, value);

		size_ += static_cast<real_size_t>(count);

		return it;
	}

	constexpr iterator insert(const_iterator pos, std::initializer_list<T> init)
	{
		if (size() + init.size() > capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector insert for initializer_list "
				"of {} elements when the vector already stores {} elements would exceed the "
				"vector's capacity of {}", init.size(), size(), capacity())};
		}

		if (pos == cend()) [[unlikely]]
		{
			detail::constexpr_uninitialized_copy_n(init.begin(), init.size(), data() + size());
			size_ += static_cast<real_size_t>(init.size());
			return begin() + std::distance(cbegin(), pos);
		}

		iterator it = begin() + std::distance(cbegin(), pos);

		detail::constexpr_uninitialized_move_n(end() - init.size(), init.size(), data() + size());
		std::ranges::move(std::make_reverse_iterator(end() - init.size()),
			std::make_reverse_iterator(it),
			std::make_reverse_iterator(end()));
		std::ranges::copy_n(init.begin(), init.size(), it);

		size_ += static_cast<real_size_t>(init.size());

		return it;
	}

	template<std::input_iterator InputIt>
	constexpr iterator insert(const_iterator pos, InputIt first, InputIt last)
		requires (std::constructible_from<T, std::iter_value_t<InputIt>>)
	{
		return insert_range(pos, std::ranges::subrange(std::move(first), std::move(last)));
	}

	template<std::ranges::input_range Range>
	constexpr iterator insert_range(const_iterator pos, Range&& range)
		requires (std::constructible_from<T, std::ranges::range_value_t<Range>>)
	{
		if constexpr (not (std::ranges::sized_range<Range> || std::ranges::forward_range<Range>))
		{
			std::ranges::copy(std::forward<Range>(range), std::inserter(*this, pos));
			return begin() + std::distance(cbegin(), pos);
		}
		{
			const size_type rsize = [&]()
			{
				if constexpr (std::ranges::sized_range<Range>)
				{
					return std::ranges::size(range);
				}
				else
				{
					return std::ranges::distance(range);
				}
			}();

			if (size() + rsize > capacity()) [[unlikely]]
			{
				throw std::length_error{ std::format("Static vector insert for range "
					"of {} elements when the vector already stores {} elements would exceed the "
					"vector's capacity of {}", rsize, size(), capacity()) };
			}

			if (pos == cend()) [[unlikely]]
			{
				detail::constexpr_uninitialized_copy_n(std::ranges::begin(range),
					rsize, data() + size());
				size_ += static_cast<real_size_t>(rsize);
				return begin() + std::distance(cbegin(), pos);
			}

			iterator it = begin() + std::distance(cbegin(), pos);
			detail::constexpr_uninitialized_move_n(end() - rsize, rsize, data() + size());
			std::ranges::move(std::make_reverse_iterator(end() - rsize),
				std::make_reverse_iterator(it),
				std::make_reverse_iterator(end()));
			std::ranges::copy_n(std::ranges::begin(range), rsize, it);

			size_ += static_cast<real_size_t>(rsize);

			return it;
		}
	}

	constexpr void push_back(const_reference value)
	{
		if (size() == capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector push_back call would "
				"exceed the vector's capacity of {}", capacity()) };
		}

		std::construct_at(data() + size(), value);
		++size_;
	}

	constexpr void push_back(value_type&& value)
	{
		if (size() == capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector push_back call would "
				"exceed the vector's capacity of {}", capacity()) };
		}

		std::construct_at(data() + size(), std::move(value));
		++size_;
	}

	template<typename ... Args>
	requires std::is_constructible_v<T, Args...>
	constexpr reference emplace_back(Args&& ... args)
	{
		if (size() == capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("Static vector emplace_back call would "
				"exceed the vector's capacity of {}", Capacity) };
		}

		std::construct_at(data() + size(), std::forward<Args>(args)...);
		++size_;

		return back();
	}

	template<std::ranges::input_range Range>
	constexpr void append_range(Range&& range)
	{
		if constexpr (std::ranges::sized_range<Range> || std::ranges::forward_range<Range>)
		{
			const size_type rsize = [&]()
			{
				if constexpr (std::ranges::sized_range<Range>)
				{
					return std::ranges::size(range);
				}
				else
				{
					return std::ranges::distance(range);
				}
			}();
			
			if (size() + rsize > capacity()) [[unlikely]]
			{
				throw std::length_error{ std::format("Static vector append_range call "
					"with a range of size {} and with already {} elements in the vector "
					"would exceed the vector's capacity of {}",
					rsize, size(), capacity()) };
			}

			detail::constexpr_uninitialized_copy_n(data() + size(),
				rsize, std::ranges::begin(range));
			size_ += static_cast<real_size_t>(rsize);
		}
		else
		{
			std::ranges::copy(std::forward<Range>(range), std::back_inserter(*this));
		}
	}

	constexpr void pop_back() noexcept
	{
		std::destroy_at(data() + size() - 1);
		--size_;
	}

	constexpr iterator erase(const_iterator pos)
		noexcept (std::is_nothrow_move_assignable_v<T>)
	{
		const size_type offset = std::distance(cbegin(), pos);
		std::ranges::move(begin() + offset + 1, end(), begin() + offset);
		pop_back();

		return begin() + offset;
	}

	constexpr iterator erase(const_iterator first, const_iterator last)
		noexcept (std::is_nothrow_move_assignable_v<T>)
	{
		const size_type offset = std::distance(cbegin(), first);
		const size_type count = std::distance(first, last);
		std::ranges::move(begin() + offset + count, end(), begin() + offset);
		
		std::destroy_n(data() + size() - count, count);

		size_ -= static_cast<real_size_t>(count);

		return begin() + offset;
	}

	constexpr size_type size() const noexcept
	{
		return static_cast<size_type>(size_);
	}

	constexpr size_type capacity() const noexcept
	{
		return Capacity;
	}

	constexpr size_type max_size() const noexcept
	{
		return capacity();
	}

	constexpr bool empty() const noexcept
	{
		return size_ == 0;
	}

	constexpr void reserve(size_type) const
	{
		/*if (count > capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("reserve request for {} elements "
				"on static_vector of max capacity of {} cannot be fulfilled.\n\n"
				"Do note that the reserve method on a static_vector is a no-op.",
				count, capacity()) };
		}*/

		// does nothing
	}

	constexpr void shrink_to_fit() const
	{
		// does nothing
	}

	constexpr void clear()
		noexcept(std::is_nothrow_destructible_v<T>)
	{
		std::destroy_n(data(), size());

		size_ = 0;
	}

	constexpr void resize(size_type count)
	{
		if (count > capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("resize request for {} elements "
				"on static_vector of max capacity of {} cannot be fulfilled.",
				count, capacity()) };
		}

		if (count < size())
		{
			std::destroy_n(data() + count, size() - count);
		}
		else if (count > size())
		{
			detail::constexpr_uninitialized_value_construct_n(
				data() + size(), count - size());
		}

		size_ = static_cast<real_size_t>(count);
	}

	constexpr void resize(size_type count, const T& value)
	{
		if (count > capacity()) [[unlikely]]
		{
			throw std::length_error{ std::format("resize request for {} elements "
				"on static_vector of max capacity of {} cannot be fulfilled.",
				count, capacity()) };
		}

		if (count < size())
		{
			std::destroy_n(data() + count, size() - count);
		}
		else if (count > size())
		{
			detail::constexpr_uninitialized_fill_n(
				data() + size(), count - size(), value);
		}

		size_ = static_cast<real_size_t>(count);
	}

	constexpr ~static_vector()
	noexcept(std::is_nothrow_destructible_v<T>)
	requires(not std::is_trivially_destructible_v<T>)
	{
		std::destroy_n(data(), size());
	}

	constexpr ~static_vector() noexcept
	requires(std::is_trivially_destructible_v<T>)
		= default;

	constexpr iterator begin() noexcept
	{
		return data();
	}

	constexpr reverse_iterator rbegin() noexcept
	{
		return std::make_reverse_iterator(begin());
	}

	constexpr iterator end() noexcept
	{
		return &data()[size()];
	}

	constexpr reverse_iterator rend() noexcept
	{
		return std::make_reverse_iterator(end());
	}

	constexpr const_iterator begin() const noexcept
	{
		return data();
	}

	constexpr const_iterator end() const noexcept
	{
		return &data()[size()];
	}

	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return std::make_reverse_iterator(cbegin());
	}

	constexpr const_reverse_iterator rend() const noexcept
	{
		return std::make_reverse_iterator(cend());
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return data();
	}

	constexpr const_iterator cend() const noexcept
	{
		return &data()[size()];
	}

	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return std::make_reverse_iterator(cbegin());
	}

	constexpr const_reverse_iterator crend() const noexcept
	{
		return std::make_reverse_iterator(cend());
	}

private:
	union {
		std::byte dummy_{};
		T data_[Capacity];
	};
	real_size_t size_ = 0;
};

template<typename T, std::size_t Size1, std::size_t Size2>
constexpr auto operator<=>(const static_vector<T, Size1>& lhs,
	const static_vector<T, Size2>& rhs)
	noexcept(noexcept(std::declval<T>() <=> std::declval<T>()))
{
	return std::ranges::lexicographical_compare(lhs, rhs);
}

template<typename T, std::size_t Capacity, typename U>
constexpr std::size_t erase(static_vector<T, Capacity>& vec, const U& value)
{
	const auto [_, it] = std::ranges::remove(vec, value);
	auto r = std::distance(it, vec.end());
	vec.erase(it, vec.end());
	return r;
}

template<typename T, std::size_t Capacity, typename Pred>
constexpr std::size_t erase(static_vector<T, Capacity>& vec, Pred pred)
{
	const auto [_, it] = std::ranges::remove_if(vec, std::move(pred));
	auto r = std::distance(it, vec.end());
	vec.erase(it, vec.end());
	return r;
}