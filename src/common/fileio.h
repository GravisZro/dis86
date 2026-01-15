#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

#include "result.h"

// simple file reader
template<typename R>
Result<R, std::error_code> read_file(const std::filesystem::path& path)
{
  std::error_code ec;
  if(!std::filesystem::exists(path, ec) || ec)
    return ec;

  const auto sz = std::filesystem::file_size(path, ec);
  if(ec)
    return ec;

  std::ifstream file(path, std::ios::in | std::ios::binary);
  R result;
  result.reserve(sz);
  file.read(reinterpret_cast<char*>(result.data()), sz);
  return result;
}
