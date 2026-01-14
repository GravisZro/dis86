#include "dynarray.h"

#include <cassert>
#include <cstdlib>

dynarray::dynarray(dynarray&& other)
{
  m_size = other.m_size;
  m_data = other.m_data;
  other.m_data = nullptr;
  other.m_size = 0;
}

dynarray& dynarray::operator =(dynarray&& other)
{
  m_size = other.m_size;
  m_data = other.m_data;
  other.m_data = nullptr;
  other.m_size = 0;
  return *this;
}

void dynarray::init(size_t sz)
{
  assert(empty());
  assert(sz > 0);
  m_data = static_cast<uint8_t*>(malloc(sz));
  assert(m_data != nullptr);
  m_size = sz;
}

void dynarray::clear(void)
{
  if(!empty())
  {
    free(m_data);
    m_data = nullptr;
    m_size = 0;
  }
}
