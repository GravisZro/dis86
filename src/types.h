#pragma once

//#include "config.h"

#include <cstdint>
#include <unistd.h>

#include <format>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>

#include "common/result.h"

namespace types
{

  struct StructRef
  {
    size_t idx;
    uint16_t size;
  };

  struct ArraySize
  {
    bool   known = false;
    size_t value = 0;
  };

  struct Type
  {
    enum type_e
    {
      Void, U8, U16, U32, I8, I16, I32,
      Array,
      Ptr,
      Struct,
      Unknown,
    } type = Unknown;

    struct array_t
    {
      std::shared_ptr<Type> typ;
      ArraySize sz;
    };

    Type(void) : type(Void) { }
    Type(uint8_t) : type(U8) { }
    Type(uint16_t) : type(U16) { }
    Type(uint32_t) : type(U32) { }
    Type(int8_t) : type(I8) { }
    Type(int16_t) : type(I16) { }
    Type(int32_t) : type(I32) { }

    Type(type_e t) : type(t) { }
    Type(const array_t& o) : type(Array), array(o) { }
    Type(const std::shared_ptr<Type>& o) : type(Ptr), ptr(o) { }
    Type(const StructRef& o) : type(Struct), structref(o) { }


    explicit constexpr operator type_e(void) const { return type; }

    array_t array;
    std::shared_ptr<Type> ptr;
    StructRef structref;

    bool is_primitive(void) const;
    std::optional<size_t> size_in_bytes(void) const;

    std::string to_str(void) const;
  };

  struct StructMember
  {
    std::string name;
    Type typ;
    uint16_t off;
  };

  struct Struct
  {
    std::string name;
    uint16_t size;
    std::vector<StructMember> members;
  };

  struct Builder
  {
    Builder(void);

    void append_struct(const Struct& s);
    std::optional<Struct> lookup_struct(const StructRef& r)const;
    Result<Type, std::string> parse_type(const std::string& s)const;
    Result<Type, std::string> parse_array_type(const std::string& s) const;

    std::vector<Struct> structs;
    std::unordered_map<std::string, Type> basetypes;
  };
}
