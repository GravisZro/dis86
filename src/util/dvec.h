#pragma once
#include <cstdint>
#include <vector>
#include <optional>

namespace dvec
{
  using DVecIndex = int64_t;

  template<typename T>
  struct Range
  {
    T start;
    T end;
  };

  // Double-ended Vector

  template<typename T>
  class DVec
  {
  public:
    DVecIndex start(void) const { return -neg.size(); }
    DVecIndex end(void) const { return pos.size(); }
    Range<DVecIndex> range(void) const { return { start(), end() }; }

    bool empty(void) const { return !start() && !end(); }
    /*
    DVecIndex push_front(T val);
    DVecIndex push_back(T val);
    std::optional<DVecIndex> last_idx(void);
    std::optional<T> last(void);
    const T& operator [](DVecIndex index) const;
    T& operator [](DVecIndex index);
  */
    DVecIndex push_front(T val)
    {
      neg.push_back(val);
      return start();
    }

    DVecIndex push_back(T val)
    {
      auto idx = end();
      pos.push_back(val);
      return idx;
    }

    std::optional<DVecIndex> last_idx(void)
    {
      if(pos.size() > 0)
        return pos.size() - 1;
      else if(neg.size() > 0)
        return -1;
      return {};
    }

    std::optional<T> last(void)
    {
      auto idx = last_idx();
      if(idx)
        return operator[](*idx);
      return {};
    }


    const T& operator [](DVecIndex index) const
    {
      if(index < 0)
        return neg[-index+1];
      else
        return pos[index];
    }

    T& operator [](DVecIndex index)
    {
      if(index < 0)
        return neg[-index+1];
      else
        return pos[index];
    }
  private:
    std::vector<T> neg;
    std::vector<T> pos;
  };
}
