#pragma once

#include <iostream>
#include <format>
#include <string_view>

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
