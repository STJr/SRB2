// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_CXXUTIL_HPP__
#define __SRB2_CXXUTIL_HPP__

#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>

#include "doomdef.h"

namespace srb2 {

template <class F>
class Finally {
public:
	explicit Finally(const F& f) noexcept : f_(f) {}
	explicit Finally(F&& f) noexcept : f_(f) {}

	Finally(Finally&& from) noexcept : f_(std::move(from.f_)), call_(std::exchange(from.call_, false)) {}

	Finally(const Finally& from) = delete;
	void operator=(const Finally& from) = delete;
	void operator=(Finally&& from) = delete;

	~Finally() noexcept {
		if (call_)
			f_();
	}

private:
	F f_;
	bool call_ = true;
};

template <class F>
Finally<std::decay_t<F>> finally(F&& f) noexcept {
	return Finally {std::forward<F>(f)};
}

struct SourceLocation {
	const char* file_name;
	unsigned int line_number;
};

#define SRB2_SOURCE_LOCATION                                                                                           \
	srb2::SourceLocation { __FILE__, __LINE__ }

#ifndef SRB2_ASSERT_HANDLER
#define SRB2_ASSERT_HANDLER srb2::IErrorAssertHandler
#endif

#if !defined(NDEBUG) || defined(PARANOIA)
// An assertion level of 2 will activate all invocations of the SRB2_ASSERT macro
#define SRB2_ASSERTION_LEVEL 2
#else
// The minimum assertion level is 1
#define SRB2_ASSERTION_LEVEL 1
#endif

/// Assert a precondition expression in debug builds.
#define SRB2_ASSERT(expr) srb2::do_assert<2, SRB2_ASSERT_HANDLER>([&] { return (expr); }, SRB2_SOURCE_LOCATION, #expr)

class IErrorAssertHandler {
public:
	static void handle(const SourceLocation& source_location, const char* expression) {
		I_Error(
			"Assertion failed at %s:%u: %s != true",
			source_location.file_name,
			source_location.line_number,
			expression
		);
	}
};

class NoOpAssertHandler {
public:
	static void handle(const SourceLocation& source_location, const char* expression) {
		(void)source_location;
		(void)expression;
	}
};

/// @brief Assert a precondition expression, aborting the application if it fails.
/// @tparam Expr
/// @tparam Level the level of this assertion; if it is less than or equal to SRB2_ASSERTION_LEVEL, this overload will
/// activate.
/// @param expr a callable which returns a bool
/// @param source_location a struct containing the source location of the assertion, e.g. SRB2_SOURCE_LOCATION
/// @param expression the expression evaluated in the expression callable
/// @param message an optional message to display for the assertion
template <unsigned int Level, class Handler, class Expr>
std::enable_if_t<(Level <= SRB2_ASSERTION_LEVEL), void>
do_assert(const Expr& expr, const SourceLocation& source_location, const char* expression = "") noexcept {
	static_assert(Level > 0, "level of an assertion must not be 0");
	if (!expr()) {
		Handler::handle(source_location, expression);
		std::abort();
	}
}

template <unsigned int Level, class, class Expr>
std::enable_if_t<(Level > SRB2_ASSERTION_LEVEL), void>
do_assert(const Expr&, const SourceLocation&, const char* = "") noexcept {
}

template <typename T>
class NotNull final {
	T ptr_;

public:
	static_assert(
		std::is_convertible_v<decltype(std::declval<T>() != nullptr), bool>,
		"T is not comparable with nullptr_t"
	);

	/// @brief Move-construct from the pointer value U, asserting that it is not null. Allows construction of a
	/// NotNull<T> from any compatible pointer U, for example with polymorphic classes.
	template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
	constexpr NotNull(U&& rhs) : ptr_(std::forward<U>(rhs)) {
		SRB2_ASSERT(ptr_ != nullptr);
	}

	/// @brief Wrap the pointer type T, asserting that the pointer is not null.
	template <typename = std::enable_if_t<!std::is_same_v<std::nullptr_t, T>>>
	constexpr NotNull(T rhs) : ptr_(std::move(rhs)) {
		SRB2_ASSERT(ptr_ != nullptr);
	}

	/// @brief Copy construction from NotNull of convertible type U. Only if the incoming pointer is NotNull already.
	template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
	constexpr NotNull(const NotNull<U>& rhs) : NotNull(rhs.get()) {
		// Value is guaranteed to be not null by construction; no assertion necessary
	}

	NotNull(const NotNull& rhs) = default;
	NotNull& operator=(const NotNull& rhs) = default;

	/// @brief Get the stored pointer.
	constexpr T get() const { return ptr_; }

	/// @brief Convert to T (the pointer type).
	constexpr operator T() const { return get(); }

	/// @brief Arrow-dereference to *T (the actual value pointed to).
	constexpr decltype(auto) operator->() const { return get(); }

	/// @brief Dereference to *T (the actual value pointed to).
	constexpr decltype(auto) operator*() const { return *get(); }

	// It is not allowed to construct NotNull<T> with nullptr regardless of T.
	NotNull(std::nullptr_t) = delete;
	NotNull& operator=(std::nullptr_t) = delete;
};

template <class T>
NotNull(T) -> NotNull<T>;

/// @brief Utility struct for combining several Callables (e.g. lambdas) into a single Callable with the call operator
/// overloaded. Use it to build a visitor for calling std::visit on variants.
/// @tparam ...Ts callable types
template <typename... Ts>
struct Overload : Ts... {
	using Ts::operator()...;
};

template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

inline void hash_combine(std::size_t& seed)
{
	(void)seed;
}

template <class T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	hash_combine(seed, std::forward<Rest>(rest)...);
}

} // namespace srb2

#endif // __SRB2_CXXUTIL_HPP__
