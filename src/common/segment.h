#ifndef SEGMENT_H
#define SEGMENT_H

#include <cstdint>
#include <cassert>
#include <unistd.h>


template<typename T>
class segment
{
  public:
    segment(void) = default;
    segment(void* data, const size_t size)
      : m_data(static_cast<T*>(data)), m_size(size) { };
    ~segment(void) { }

    T& operator [](size_t index)
    {
      assert(index < size());
      return m_data[index];
    }

    T operator [](size_t index) const
    {
      assert(index < size());
      return m_data[index];
    }

    constexpr bool valid(void) const { return m_data != nullptr; }
    constexpr T* data(void) const { return m_data; }
    constexpr size_t size(void) const { return m_size;}
    constexpr void* after(void) const { return reinterpret_cast<void*>(data() + size()); }
  private:
    T* m_data = nullptr;
    size_t m_size = 0;
};

#endif // SEGMENT_H
