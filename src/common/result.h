#ifndef RESULT_H
#define RESULT_H

#include <functional>
#include <type_traits>

template<typename T> struct deref { using type = T; };
template<typename T> struct deref<T*> { using type = T; };


template<typename Type, typename Err>
class Result
{
public:
  Result(Type v) : m_ok(true), m_value(v) { }
  Result(Err e) : m_ok(false), m_error(e) { }

  constexpr bool is_ok (void) { return m_ok; }
  constexpr bool is_err(void) { return !m_ok; }
  constexpr const Type& value(void) const { return m_value; }
  constexpr const Err& error(void) const { return m_error; }
  constexpr Type& value(void) { return m_value; }
  constexpr Err& error(void) { return m_error; }

  constexpr       Type& operator  *(void)       { return m_value; }
  constexpr const Type& operator  *(void) const { return m_value; }
  constexpr       Type  operator ->(void)       { return m_value; }
  constexpr const Type  operator ->(void) const { return m_value; }

  template <typename R = bool>
  bool is_ok_and(const std::function<R(Type&)> &func, R rval = true)
  {
    if(is_ok())
      return func(m_value) == rval;
    return false;
  }

  template <typename R = bool>
  bool is_err_and(const std::function<R(Err&)> &func, R rval = true)
  {
    if(is_err())
      return func(m_error) == rval;
    return false;
  }
private:
  bool m_ok;
  Type m_value;
  Err  m_error;
};

#endif // RESULT_H
