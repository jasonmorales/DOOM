export module nstd.vecs;

import std;
import nstd.numbers;
import nstd.traits;
import nstd.strings;

export namespace nstd {

template<typename T>
class v2_t
{
public:
	using value_type = T;

	T x = 0;
	T y = 0;

	/*constexpr ~v2_t() noexcept = default;
	constexpr v2_t() noexcept = default;
	constexpr v2_t(const v2_t&) noexcept = default;
	constexpr v2_t(v2_t&&) noexcept = default;*/
	//constexpr explicit v2_t(T p[2]) noexcept : x{p[0]}, y{p[1]} {}

	/*constexpr v2_t& operator=(const v2_t&) noexcept = default;
	constexpr v2_t& operator=(v2_t&&) noexcept = default;*/

	template<typename TO> v2_t<TO> cast() const noexcept { return {static_cast<TO>(x), static_cast<TO>(y)}; }
	template<typename TO = T> v2_t<TO> round() const noexcept { return {round_to<TO>(x), round_to<TO>(y)}; }
	template<typename TO = T> v2_t<TO> floor() const noexcept { return {floor_to<TO>(x), floor_to<TO>(y)}; }
	template<typename TO = T> v2_t<TO> ceil() const noexcept { return {ceil_to<TO>(x), ceil_to<TO>(y)}; }

	constexpr v2_t xx() const noexcept { return {x, x}; }
	constexpr v2_t yy() const noexcept { return {y, y}; }
	constexpr v2_t yx() const noexcept { return {y, x}; }

	/*v3_t<T> xxx() const { return {x, x, x}; }
	v3_t<T> yyy() const { return {y, y, y}; }
	v3_t<T> _xy() const { return {0, x, y}; }
	v3_t<T> x_y() const { return {x, 0, y}; }
	v3_t<T> xy_() const { return {y, y, 0}; }

	v4_t<T> xxxx() const { return {x, x, x, x}; }
	v4_t<T> yyyy() const { return {y, y, x, y}; }
	v4_t<T> xy__() const { return {x, y, 0, 0}; }
	v4_t<T> x_y_() const { return {x, 0, y, 0}; }
	v4_t<T> x__y() const { return {x, 0, 0, y}; }
	v4_t<T> _xy_() const { return {0, x, y, 0}; }
	v4_t<T> _x_y() const { return {0, x, 0, y}; }
	v4_t<T> __xy() const { return {0, 0, x, y}; }*/

	static v2_t from_angle(number auto a) noexcept { return { cos(a), sin(a) }; }
	template<typename AS = T> constexpr AS get_angle() const noexcept { return atan2(y, x); }

	constexpr bool is_zero() const noexcept { return x == 0 && y == 0; }
	constexpr void clear() noexcept { x = y = 0; }

	v2_t& SetLength(T f) { if (x != 0.f || y != 0.f) { T scale(f / sqrt(x * x + y * y)); x *= scale; y *= scale; } return *this; }

	template<typename TO = T>
	TO length() const { return ::rcast<TO>(sqrt(x * x + y * y)); }

	T GetLengthSq() const { return x * x + y * y; }
	v2_t& Clamp(double length)
	{
		auto currentSqr = GetLengthSq();
		auto clampSqr = sqr(length);
		if (currentSqr > clampSqr)
		{
			auto ratio = length / sqrt(currentSqr);
			x *= ratio;
			y *= ratio;
		}

		return *this;
	}

	T normalize()
	{
		static_assert(std::is_floating_point_v<T>, "Attempting to normalize integral vector");
		auto length = sqrt(x * x + y * y);
		x /= length;
		y /= length;
		return length;
	}

	v2_t& MakeNormalized() { T length(sqrt(x * x + y * y)); x /= length; y /= length; return *this; }

	template<typename TO = best_float_t<T>>
	constexpr v2_t<TO> get_normal() const
	{
		auto length = sqrt(static_cast<TO>(x * x + y * y));
		return {static_cast<TO>(x / length), static_cast<TO>(y / length)};
	}

	v2_t& MakeOrthogonal() { Swap(x, y); y = -y; return *this; }
	v2_t ortho() const { return {-y, x}; }

	T dot(v2_t v) const { return x * v.x + y * v.y; }

	v2_t& Rotate(T a);
	v2_t GetRotated(float a) const;
	v2_t get_rotated(T a, v2_t p) const;

	void Rotate(const v2_t& o, T a);

	constexpr v2_t operator-() const noexcept { return {-x, -y}; }

	constexpr auto operator+(number auto n) const noexcept { return v2_t{x + n, y + n}; }
	constexpr auto operator-(number auto n) const noexcept { return v2_t{x - n, y - n}; }
	constexpr auto operator*(number auto n) const noexcept { return v2_t{x * n, y * n}; }
	constexpr auto operator/(number auto n) const noexcept { return v2_t{x / n, y / n}; }

	template<typename V> v2_t<decltype(T() + V())> operator+(v2_t<V> v) const { return { x + v.x, y + v.y }; }
	template<typename V> v2_t<decltype(T() - V())> operator-(v2_t<V> v) const { return { x - v.x, y - v.y }; }
	template<typename V> v2_t<decltype(T() * V())> operator*(v2_t<V> v) const { return { x * v.x, y * v.y }; }
	template<typename V> v2_t<decltype(T() / V())> operator/(v2_t<V> v) const { return { x / v.x, y / v.y }; }

	template<typename N> v2_t& operator+=(N n) { x = ::rcast<T>(x + n); y = ::rcast<T>(y + n); return *this; }
	template<typename N> v2_t& operator-=(N n) { x = ::rcast<T>(x - n); y = ::rcast<T>(y - n); return *this; }
	template<typename N> v2_t& operator*=(N n) { x = ::rcast<T>(x * n); y = ::rcast<T>(y * n); return *this; }
	template<typename N> v2_t& operator/=(N n) { x = ::rcast<T>(x / n); y = ::rcast<T>(y / n); return *this; }

	template<typename V> v2_t& operator+=(v2_t<V> v) { x = ::rcast<T>(x + v.x); y = ::rcast<T>(y + v.y); return *this; }
	template<typename V> v2_t& operator-=(v2_t<V> v) { x = ::rcast<T>(x - v.x); y = ::rcast<T>(y - v.y); return *this; }
	template<typename V> v2_t& operator*=(v2_t<V> v) { x = ::rcast<T>(x * v.x); y = ::rcast<T>(y * v.y); return *this; }
	template<typename V> v2_t& operator/=(v2_t<V> v) { x = ::rcast<T>(x / v.x); y = ::rcast<T>(y / v.y); return *this; }

	template<integral V>
		requires std::is_integral_v<T>
	constexpr bool operator==(v2_t<V> v) const { return x == v.x && y == v.y; }

	template<typename F> static constexpr F EPSILON = static_cast<F>(0.0001);

	template<typename V>
		requires (!std::is_integral_v<T>) || (!std::is_integral_v<V>)
	constexpr bool operator==(v2_t<V> v) const
	{
		using comparison_t = decltype(T() - V());
		return abs(x - v.x) < EPSILON<comparison_t> && abs(y - v.y) < EPSILON<comparison_t>;
	}

	template<typename V> constexpr bool operator!=(v2_t<V> v) const { return !(*this == v); }

	template<class URNG>
	static v2_t GetRandomValueInRange(const v2_t& min, const v2_t& max, URNG& rand)
	{
		return {
			::GetRandomValueInRange(min.x, max.x, rand),
			::GetRandomValueInRange(min.y, max.y, rand) };
	}

	static v2_t DefaultValue() { return {}; }

	static constexpr v2_t from_string(::string_view str);

	static const v2_t Zero;
	static const v2_t Left;
	static const v2_t Right;
	static const v2_t Up;
	static const v2_t Down;
};

using v2f = v2_t<float>;
using v2d = v2_t<double>;
using v2i = v2_t<int32>;

template<typename T> const v2_t<T> v2_t<T>::Zero = { 0, 0 };
template<typename T> const v2_t<T> v2_t<T>::Left = { -1, 0 };
template<typename T> const v2_t<T> v2_t<T>::Right = { 1, 0 };
template<typename T> const v2_t<T> v2_t<T>::Up = { 0, -1 };
template<typename T> const v2_t<T> v2_t<T>::Down = { 0, 1 };

} // export namespace nstd