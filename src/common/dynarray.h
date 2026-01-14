#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <cstdint>
#include <unistd.h>
#include <cassert>

#include "segment.h"

class dynarray
{
public:
  dynarray(void) = default;
  dynarray(size_t sz) { init(sz); }
  ~dynarray(void) { clear(); }

  dynarray(dynarray&& other);
  dynarray& operator =(dynarray&& other);

  void init(size_t sz);
  void clear(void);

  uint8_t operator [](size_t idx) const
  {
    assert(idx < m_size);
    return m_data[idx];
  }

  constexpr size_t   size (void) const { return m_size; }
  template<typename T = uint8_t> constexpr T* data (void) const { return reinterpret_cast<T*>(m_data); }
  template<typename T = uint8_t> constexpr T* dataAtOffset (size_t offset) const
  {
    assert(offset < m_size);
    return reinterpret_cast<T*>(m_data + offset);
  }

  template<typename T = uint8_t>
  segment<T> segment(size_t offset, size_t length)
  {
    assert(offset + length <= m_size);
    return ::segment<T>(dataAtOffset(offset), length);
  }

  constexpr bool     empty(void) const { return m_data == nullptr; }
  constexpr operator bool(void) const { return !empty(); }
private:
  size_t m_size = 0;
  uint8_t* m_data = nullptr;
};

#endif // DYNARRAY_H
