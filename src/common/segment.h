#ifndef SEGMENT_H
#define SEGMENT_H

#include <cstdint>
#include <cassert>

template<typename T>
class segment
{
  public:
    segment(void) = default;
    segment(void* data, const std::size_t size)
      : m_data(static_cast<T*>(data)), m_size(size) { };
    ~segment(void) { }

    T& operator [](std::size_t index)
    {
      assert(index < size());
      return m_data[index];
    }

    T operator [](std::size_t index) const
    {
      assert(index < size());
      return m_data[index];
    }

    constexpr std::size_t size(void) const { return m_size;}
    constexpr bool       valid(void) const { return m_data != nullptr; }

    template<typename R = T>
    constexpr R* data(void) const { return reinterpret_cast<R*>(m_data); }

    template<typename R = T>
    constexpr R* ptr_at(std::size_t offset) const { return reinterpret_cast<R*>(data() + offset); }

    template<typename R = T>
    constexpr R* after(void) const { return ptr_at<R>(size()); }
  private:
    T* m_data = nullptr;
    std::size_t m_size = 0;
};

#endif // SEGMENT_H
