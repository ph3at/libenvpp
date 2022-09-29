#pragma once

#include <utility>
#include <variant>

namespace env {

template <typename E>
class unexpected {
  public:
	constexpr unexpected(const unexpected&) = default;
	constexpr unexpected(unexpected&&) = default;

	template <typename Err = E>
	constexpr explicit unexpected(Err&& e) : m_error(std::forward<Err>(e))
	{}

	[[nodiscard]] constexpr const E& error() const& noexcept { return m_error; }
	[[nodiscard]] constexpr E& error() & noexcept { return m_error; }
	[[nodiscard]] constexpr const E&& error() const&& noexcept { return std::move(m_error); }
	[[nodiscard]] constexpr E&& error() && noexcept { return std::move(m_error); }

  private:
	E m_error;
};

template <typename T, typename E>
class expected {
  public:
	using value_type = T;
	using error_type = E;
	using unexpected_type = unexpected<E>;
	template <typename U>
	using rebind = expected<U, error_type>;

	constexpr expected() noexcept : expected(T()) {}

	constexpr expected(const expected&) = default;
	constexpr expected(expected&&) noexcept = default;

	constexpr expected& operator=(const expected&) = default;
	constexpr expected& operator=(expected&&) noexcept = default;

	template <typename U = T>
	constexpr explicit expected(U&& value) : m_value_or_error(static_cast<T>(std::forward<U>(value)))
	{}

	template <typename G>
	constexpr explicit expected(const unexpected<G>& e) : m_value_or_error(unexpected_type{static_cast<E>(e.error())})
	{}

	template <typename G>
	constexpr explicit expected(unexpected<G>&& e)
	    : m_value_or_error(unexpected_type{static_cast<E>(std::move(e).error())})
	{}

	template <typename U = T>
	constexpr expected& operator=(U&& value)
	{
		m_value_or_error = static_cast<T>(std::forward<U>(value));
		return *this;
	}

	template <typename G>
	constexpr expected& operator=(const unexpected<G>& e)
	{
		m_value_or_error = unexpected_type{static_cast<E>(e.error())};
		return *this;
	}

	template <typename G>
	constexpr expected& operator=(unexpected<G>&& e)
	{
		m_value_or_error = unexpected_type{static_cast<E>(std::move(e).error())};
		return *this;
	}

	[[nodiscard]] constexpr const T* operator->() const noexcept { return std::get_if<T>(&m_value_or_error); }
	[[nodiscard]] constexpr T* operator->() noexcept { return std::get_if<T>(&m_value_or_error); }
	[[nodiscard]] constexpr const T& operator*() const& noexcept { return std::get<T>(m_value_or_error); }
	[[nodiscard]] constexpr T& operator*() & noexcept { return std::get<T>(m_value_or_error); }
	[[nodiscard]] constexpr const T&& operator*() const&& noexcept { return std::get<T>(std::move(m_value_or_error)); }
	[[nodiscard]] constexpr T&& operator*() && noexcept { return std::get<T>(std::move(m_value_or_error)); }

	[[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }
	[[nodiscard]] constexpr bool has_value() const noexcept { return std::holds_alternative<T>(m_value_or_error); }

	[[nodiscard]] constexpr T& value() & { return **this; }
	[[nodiscard]] constexpr const T& value() const& { return **this; }
	[[nodiscard]] constexpr T&& value() && { return std::move(**this); }
	[[nodiscard]] constexpr const T&& value() const&& { return std::move(**this); }

	[[nodiscard]] constexpr const E& error() const& noexcept
	{
		return std::get<unexpected_type>(m_value_or_error).error();
	}
	[[nodiscard]] constexpr E& error() & noexcept { return std::get<unexpected_type>(m_value_or_error).error(); }
	[[nodiscard]] constexpr const E&& error() const&& noexcept
	{
		return std::get<unexpected_type>(std::move(m_value_or_error)).error();
	}
	[[nodiscard]] constexpr E&& error() && noexcept
	{
		return std::get<unexpected_type>(std::move(m_value_or_error)).error();
	}

	template <typename U>
	[[nodiscard]] constexpr T value_or(U&& default_value) const&
	{
		return has_value() ? **this : static_cast<T>(std::forward<U>(default_value));
	}
	template <typename U>
	[[nodiscard]] constexpr T value_or(U&& default_value) &&
	{
		return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
	}

  private:
	std::variant<T, unexpected_type> m_value_or_error;
};

} // namespace env
