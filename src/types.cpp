#include "types.h"

#include <cassert>
namespace types
{
  using namespace std::string_literals;

  bool Type::is_primitive(void) const
  {
    return type == type_e::Void ||
           type == type_e::U8  ||
           type == type_e::U16 ||
           type == type_e::U32 ||
           type == type_e::I8  ||
           type == type_e::I16 ||
           type == type_e::I32 ||
           type == type_e::Unknown;
  }

  std::optional<std::size_t> Type::size_in_bytes(void) const
  {
    switch(type)
    {
      case type_e::I8:
      case type_e::U8: return 1;
      case type_e::I16:
      case type_e::U16: return 2;
      case type_e::I32:
      case type_e::U32: return 4;
      case type_e::Array:
        if(array.sz.known && array.typ->size_in_bytes().has_value())
          return array.typ->size_in_bytes().value() *
                 array.sz.value;
        return 0;
      case type_e::Struct:
        return structref.size;

      case type_e::Ptr: return 2; // on 8086 this a (seg:off) pair
      default:
        break;
    }
    return {};
  }

  std::string Type::to_string(void) const
  {
    switch(type)
    {
      case type_e::Void: return "void";
      case type_e::I8:   return "i8";
      case type_e::U8:   return "u8";
      case type_e::I16:  return "i16";
      case type_e::U16:  return "u16";
      case type_e::I32:  return "i32";
      case type_e::U32:  return "u32";

      case type_e::Array:
        {
          assert(array.typ);
          std::string val = array.typ->to_string() + "[";
          if(array.sz.known)
            val += std::to_string(array.sz.value);
          val += "]";
          return val;
        }
      case type_e::Ptr:
      case type_e::Struct:
        return std::format("struct_id_{}", structref.idx);
      case type_e::Unknown:
        return "?unknown_type?";
    }
    return "Invalid value!";
  }

  Builder::Builder(void)
  {
    basetypes.emplace("void", Type::Void);
    basetypes.emplace("u8",   Type::U8);
    basetypes.emplace("u16",  Type::U16);
    basetypes.emplace("u32",  Type::U32);
    basetypes.emplace("i8",   Type::I8);
    basetypes.emplace("i16",  Type::I16);
    basetypes.emplace("i32",  Type::I32);
  }


  void Builder::append_struct(const Struct& s)
  {
    StructRef r { structs.size(), s.size };
    structs.push_back(s);
    basetypes.emplace(s.name, r);
  }

  std::optional<Struct> Builder::lookup_struct(const StructRef& r) const
  {
    if(r.idx < structs.size())
      return structs.at(r.idx);
    return {};
  }

  Result<Type, std::string> Builder::parse_type(const std::string& s) const
  {
    if(basetypes.contains(s))
      return basetypes.at(s);
    auto v = parse_array_type(s);
    if(v.is_err())
      return std::format("Failed to parse type: '{}' | Error: {}", s, v.error());
    return v.value();
  }


  Result<Type, std::string> Builder::parse_array_type(const std::string& s) const
  {
    std::size_t array_start = s.find('[');
    std::size_t array_end = s.find(']', array_start);
    if(array_start == std::string::npos)
      return "No opening array bracket"s;
    if(array_end == std::string::npos)
      return "No closing array bracket"s;
    if(array_end != s.size() - 1)
      return "Closing array bracket isn't at the end of the type"s;

    std::string base_str = s.substr(0, array_start);
    std::string size_str = s.substr(array_start + 1, array_end);

    auto base = parse_type(base_str);
    if(base.is_err())
      return base.error();
    std::shared_ptr<Type> base_type = std::make_shared<Type>(base.value());
    std::optional<std::size_t> sz;
    if(!size_str.empty())
    {
      try { sz = std::stoul(size_str, nullptr, 0); }
      catch (...)
        { return std::format("Cannot parse array size: {}",  size_str); }
    }
    if(!sz)
      return { Type::array_t { base_type, ArraySize { false, 0 } } };
    return { Type::array_t { base_type, ArraySize { true, sz.value() } } };
  }
}
