#ifndef EXPANDED_BASE_H
#define EXPANDED_BASE_H

template<typename T>
struct expanded_base
{
  expanded_base(T v = 0) : value(v) { }
  expanded_base(const expanded_base& o) = default;
  expanded_base(expanded_base&& o) = default;

  constexpr bool operator ==(expanded_base& o) const { return value == o.value; }
  constexpr bool operator !=(expanded_base& o) const { return value != o.value; }

  constexpr expanded_base operator +(T o) const { return value + o; }
  constexpr expanded_base operator -(T o) const { return value - o; }
  constexpr expanded_base operator *(T o) const { return value * o; }
  constexpr expanded_base operator /(T o) const { return value / o; }

  constexpr expanded_base& operator  =(T o) const { value  = o; return *this; }
  constexpr expanded_base& operator +=(T o) const { value += o; return *this; }
  constexpr expanded_base& operator -=(T o) const { value -= o; return *this; }
  constexpr expanded_base& operator *=(T o) const { value *= o; return *this; }
  constexpr expanded_base& operator /=(T o) const { value /= o; return *this; }

  constexpr operator T(void) const { return value; }
  T value;
};

#endif // EXPANDED_BASE_H
