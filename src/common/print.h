#pragma once

#include <iostream>
#include <format>
#include <string_view>
#include <cassert>

template<typename... Args>
void println(std::string_view fmt, Args... args)
{
  std::cout << std::format(fmt, args...) << std::endl;
  std::cout.flush();
}

template<typename... Args>
void print(std::string_view fmt, Args... args)
{
  std::cout << std::format(fmt, args...);
  std::cout.flush();
}

template<typename... Args>
void eprintln(std::string_view fmt, Args... args)
{
  std::cerr << std::format(fmt, args...) << std::endl;
  std::cerr.flush();
}

template<typename... Args>
void eprint(std::string_view fmt, Args... args)
{
  std::cerr << std::format(fmt, args...);
  std::cerr.flush();
}



template<typename... Args>
void panic(std::string_view fmt, Args... args)
{
  println(fmt, args...);
  assert(false);
}
