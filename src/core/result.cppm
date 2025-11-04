module;

#include <cassert>
#include <string>
#include <string_view>
#include <utility>

export module core:result;

export import :types;

export namespace stl {

struct success_tag_t {
	explicit constexpr success_tag_t() noexcept = default;
};
struct error_tag_t {
	explicit constexpr error_tag_t() noexcept = default;
};

constexpr success_tag_t success{};
constexpr error_tag_t error{};

template <typename T = u32, typename Et = std::string> class result {
  public:
	using value_type = T;
	using error_type = Et;

  private:
	bool _is_ok{false};
	union storage_union {
		value_type value;
		error_type error;
		constexpr storage_union() noexcept {} // pass in values yourself
		~storage_union() noexcept {}		  // no-op, destroy explicitly
	} _storage;
	value_type *value_ptr() noexcept { return &_storage.value; }
	const value_type *value_ptr() const noexcept { return &_storage.value; }
	error_type *error_ptr() noexcept { return &_storage.error; }
	const error_type *error_ptr() const noexcept { return &_storage.error; }
	void destroy_active() noexcept {
		if (_is_ok) {
			if constexpr (!std::is_trivially_destructible_v<value_type>)
				value_ptr()->~value_type();
		} else {
			if constexpr (!std::is_trivially_destructible_v<error_type>)
				error_ptr()->~error_type();
		}
	}

  public:
	template <typename U = value_type, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
	explicit result() noexcept(std::is_nothrow_default_constructible_v<value_type>) : _is_ok(true) {
		::new (value_ptr()) value_type();
	}

	template <typename... Args> explicit result(success_tag_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>) : _is_ok(true) {
		::new (value_ptr()) value_type(std::forward<Args>(args)...);
	}

	template <typename... Args> explicit result(error_tag_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<error_type, Args...>) : _is_ok(false) {
		::new (error_ptr()) error_type(std::forward<Args>(args)...);
	}

	template <typename TT = value_type, typename EE = error_type, typename = std::enable_if_t<std::is_copy_constructible_v<TT> && std::is_copy_constructible_v<EE>>>
	result(const result &other) noexcept(std::is_nothrow_copy_constructible_v<value_type> && std::is_nothrow_copy_constructible_v<error_type>) : _is_ok(other._is_ok) {
		if (other._is_ok) {
			::new (value_ptr()) value_type(*other.value_ptr());
		} else {
			::new (error_ptr()) error_type(*other.error_ptr());
		}
	}

	template <typename TT = value_type, typename EE = error_type, typename = std::enable_if_t<std::is_move_constructible_v<TT> && std::is_move_constructible_v<EE>>>
	result(result &&other) noexcept(std::is_nothrow_move_constructible_v<value_type> && std::is_nothrow_move_constructible_v<error_type>) : _is_ok(other._is_ok) {
		if (other._is_ok) {
			::new (value_ptr()) value_type(std::move(*other.value_ptr()));
		} else {
			::new (error_ptr()) error_type(std::move(*other.error_ptr()));
		}
	}

	~result() noexcept { destroy_active(); }

	// implicit conversions
	result(const value_type &v) noexcept(std::is_nothrow_copy_constructible_v<value_type>) : _is_ok(true) { ::new (value_ptr()) value_type(v); }
	result(value_type &&v) noexcept(std::is_nothrow_move_constructible_v<value_type>) : _is_ok(true) { ::new (value_ptr()) value_type(std::move(v)); }

	template <typename TT = T, typename EE = error_type, typename = std::enable_if_t<std::is_copy_constructible_v<TT> && std::is_copy_constructible_v<EE>>>
	result &operator=(const result &other) noexcept(std::is_nothrow_copy_constructible_v<value_type> && std::is_nothrow_copy_constructible_v<error_type>) {
		if (this == &other)
			return *this;

		// If the active variant is the same, assign into it if assignable; otherwise reconstruct.
		if (_is_ok && other._is_ok) {
			if constexpr (std::is_copy_assignable_v<value_type>) {
				*value_ptr() = *other.value_ptr();
			} else {
				// reconstruct
				destroy_active();
				::new (value_ptr()) T(*other.value_ptr());
			}
		} else if (!_is_ok && !other._is_ok) {
			if constexpr (std::is_copy_assignable_v<error_type>) {
				*error_ptr() = *other.error_ptr();
			} else {
				destroy_active();
				::new (error_ptr()) error_type(*other.error_ptr());
			}
		} else {
			// different active variant -> destroy current and copy-construct other's active member
			destroy_active();
			_is_ok = other._is_ok;
			if (_is_ok)
				::new (value_ptr()) T(*other.value_ptr());
			else
				::new (error_ptr()) error_type(*other.error_ptr());
		}
		return *this;
	}

	template <typename TT = T, typename EE = error_type, typename = std::enable_if_t<std::is_move_constructible_v<TT> && std::is_move_constructible_v<EE>>>
	result &operator=(result &&other) noexcept(std::is_nothrow_move_constructible_v<value_type> && std::is_nothrow_move_constructible_v<error_type>) {
		if (this == &other)
			return *this;

		if (_is_ok && other._is_ok) {
			if constexpr (std::is_move_assignable_v<value_type>) {
				*value_ptr() = std::move(*other.value_ptr());
			} else {
				destroy_active();
				::new (value_ptr()) T(std::move(*other.value_ptr()));
			}
		} else if (!_is_ok && !other._is_ok) {
			if constexpr (std::is_move_assignable_v<error_type>) {
				*error_ptr() = std::move(*other.error_ptr());
			} else {
				destroy_active();
				::new (error_ptr()) error_type(std::move(*other.error_ptr()));
			}
		} else {
			destroy_active();
			_is_ok = other._is_ok;
			if (_is_ok)
				::new (value_ptr()) T(std::move(*other.value_ptr()));
			else
				::new (error_ptr()) error_type(std::move(*other.error_ptr()));
		}
		return *this;
	}

	bool has_value() const noexcept { return _is_ok; }
	bool has_error() const noexcept { return !_is_ok; }
	explicit operator bool() const noexcept { return _is_ok; }
	T &value() & {
		assert(_is_ok && "accessing value when result holds error");
		return *value_ptr();
	}
	const T &value() const & {
		assert(_is_ok && "accessing value when result holds error");
		return *value_ptr();
	}
	T &&value() && {
		assert(_is_ok && "accessing value when result holds error");
		return std::move(*value_ptr());
	}

	error_type &error() & {
		assert(!_is_ok && "accessing error when result holds value");
		return *error_ptr();
	}
	const error_type &error() const & {
		assert(!_is_ok && "accessing error when result holds value");
		return *error_ptr();
	}
	error_type &&error() && {
		assert(!_is_ok && "accessing error when result holds value");
		return std::move(*error_ptr());
	}

	value_type *operator->() {
		assert(_is_ok);
		return value_ptr();
	}
	const value_type *operator->() const {
		assert(_is_ok);
		return value_ptr();
	}
	value_type &operator*() & { return value(); }
	const value_type &operator*() const & { return value(); }

	template <typename... Args> void emplace_value(Args &&...args) {
		if (_is_ok) {
			if constexpr (!std::is_trivially_destructible_v<value_type>)
				value_ptr()->~value_type();
			::new (value_ptr()) value_type(std::forward<Args>(args)...);
		} else {
			if constexpr (!std::is_trivially_destructible_v<error_type>)
				error_ptr()->~error_type();
			::new (value_ptr()) value_type(std::forward<Args>(args)...);
			_is_ok = true;
		}
	}

	template <typename... Args> void emplace_error(Args &&...args) {
		if (!_is_ok) {
			if constexpr (!std::is_trivially_destructible_v<error_type>)
				error_ptr()->~error_type();
			::new (error_ptr()) error_type(std::forward<Args>(args)...);
		} else {
			if constexpr (!std::is_trivially_destructible_v<value_type>)
				value_ptr()->~value_type();
			::new (error_ptr()) error_type(std::forward<Args>(args)...);
			_is_ok = false;
		}
	}
};

template <typename T = u32> [[nodiscard]] inline result<T> make_error(std::string_view msg) { return result<T>(error, msg); }

template <typename... Args> [[nodiscard]] inline result<> result_success(Args &&...args) { return result<>(success, std::forward(args)...); }

} // namespace stl