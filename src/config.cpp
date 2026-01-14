#include "config.h"

#include <format>
#include <charconv>
#include <iostream>
#include <fstream>

namespace config
{
  std::optional<Func> Config::func_lookup(SegOff_t addr) const
  {
    // TODO: Consider something better than linear search
    for(auto& f : funcs)
    {
      if(addr == f.start)
        return f;

        // matches as an overlay func?
      if(!f.overlay)
        continue;
      auto& overlay = *f.overlay;
      if (Seg_t num = { Seg_t::Overlay, addr.seg };
          num == overlay.num && addr.off == overlay.start)
        return f;
    }
    return {};
  }

  std::optional<Indirect> Config::indirect_lookup(SegOff_t addr) const
  {
    // TODO: Consider something better than linear search
    for(auto& i : indirects)
    {
      if(addr == i.addr)
        return i;
    }
    return {};
  }

  std::optional<Func> Config::func_lookup_by_name(const std::string& name) const
  {
    // TODO: Consider something better than linear search
    for(auto& f : funcs)
    {
      if(name == f.name)
        return f;
    }
    return {};
  }

  std::optional<TextSectionRegion> Config::text_region_lookup_by_start_addr(SegOff_t addr) const
  {
    // TODO: Consider something better than linear search
    for(auto& r : text_section)
      if(addr == r.start)
          return r;
    return {};
  }

  std::optional<TextSectionRegion> Config::text_region_lookup_by_access(SegOff_t addr) const
  {
    // TODO: Consider something better than linear search
    for(auto& r  : text_section)
      if(r.access.has_value() && *r.access == addr)
        return r;
    return {};
  }

  std::optional<TextSectionRegion> Config::text_region_lookup(SegOff_t start_addr, SegOff_t access) const
  {
    if(auto r = text_region_lookup_by_start_addr(start_addr);
        r.has_value())
      return *r;
    return text_region_lookup_by_access(access);
  }

  // simple file reader
  Result<std::string, std::error_code> read_file(const std::filesystem::path& path)
  {
    std::error_code ec;
    if(!std::filesystem::exists(path, ec) || ec)
      return ec;

    const auto sz = std::filesystem::file_size(path, ec);
    if(ec)
      return ec;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    std::string result(sz, '\0');
    file.read(result.data(), sz);
    return result;
  }


  Result<Config, std::string> Config::from_path(const std::filesystem::path& path)
  {
    Config cfg;

    auto data = read_file(path);
    if(data.is_err())
      return data.error().message();

    bsl::error_e error = bsl::success;
    std::shared_ptr<bsl::node_t> root = bsl::parse_new(*data, &error);

    auto rval = cfg.parse_structs(root); // Important for this to go first and build types
    if(rval.is_err())
      return rval.error();

    rval = cfg.parse_functions(root);
    if(rval.is_err())
      return rval.error();

    rval = cfg.parse_globals(root);
    if(rval.is_err())
      return rval.error();

    rval = cfg.parse_text_section(root);
    if(rval.is_err())
      return rval.error();

    return cfg;
  }

  Result<std::nullptr_t, std::string> Config::parse_structs(std::shared_ptr<bsl::node_t> root)
  {
    auto topnode = root->get_node("dis86.structures");
    if(!topnode)
      return std::string("Failed to get the structures node");

    for(const auto& kv : topnode->kv_arr)
    {
      auto struct_props = kv.as_node();
      if(!struct_props)
        return std::string("Expected structure properties");
      auto sz = struct_props->get_str("size");
      if(!sz)
        return std::format<"No function 'size' property for '{}'">(kv.key);

      uint16_t size = 0;
      if (auto result = std::from_chars(sz->data(), sz->data() + sz->size(), size);
          result.ec == std::errc::invalid_argument)
        return std::format<"Expected uint16_t for '{}.start', got '{}'">(kv.key, *sz);

      auto members = struct_props->get_node("members");
      if(!members)
        return std::format<"Expected {}.members node">(kv.key);

      std::vector<StructMember> member_data;
      for(const auto& mbr_kv : members->kv_arr)
      {
        auto member_node = mbr_kv.as_node();
        if(!member_node)
          return std::format<"Expected member properties for {}.members.{}">(kv.key, mbr_kv.key);

        auto off_str = member_node->get_str("off");
        if(!off_str)
          return std::format<"No 'off' property for '{}.members.{}'">(kv.key, mbr_kv.key);

        auto type_str = member_node->get_str("type");
        if(!type_str)
          return std::format<"No 'type' property for '{}.members.{}'">(kv.key, mbr_kv.key);

        uint16_t off = 0;
        if (auto result = std::from_chars(off_str->data(), off_str->data() + off_str->size(), off);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t hex for '{}.members.{}.off', got '{}'">(kv.key, mbr_kv.key, *off_str);

        auto typ = type_builder.parse_type(*type_str);
        if(typ.is_err())
          return typ.error();
        member_data.emplace_back(StructMember { mbr_kv.key, typ.value(), off });
      }
      auto s = Struct { kv.key, size, member_data };
      type_builder.append_struct(s);
      structs.push_back(s);
    }
    return { nullptr };
  }

  Result<std::nullptr_t, std::string> Config::parse_functions(std::shared_ptr<bsl::node_t> root)
  {
    auto topnode = root->get_node("dis86.structures");
    if(!topnode)
      return std::string("Failed to get the functions node");

    for(const auto& kv : topnode->kv_arr)
    {
      auto func_props = kv.as_node();
      if(!func_props)
        return std::string("Expected function properties");

      auto start_str = func_props->get_str("start");
      if(!start_str)
        return std::format<"No function 'start' property for '{}'">(kv.key);

      auto end_str = func_props->get_str("end");
      if(!end_str)
        return std::format<"No function 'end' property for '{}'">(kv.key);

      auto mode_str = func_props->get_str("mode");
      if(!mode_str)
        return std::format<"No function 'mode' property for '{}'">(kv.key);

      auto ret_str = func_props->get_str("ret");
      if(!ret_str)
        return std::format<"No function 'ret' property for '{}'">(kv.key);

      auto args_str = func_props->get_str("args");
      if(!args_str)
        return std::format<"No function 'args' property for '{}'">(kv.key);

      auto dont_pop_args = func_props->get_str("dont_pop_args");
      auto indirect      = func_props->get_str("indirect_call_location");
      auto overlay_num   = func_props->get_str("overlay_num");
      auto overlay_start = func_props->get_str("overlay_start");
      auto overlay_end   = func_props->get_str("overlay_end");
      auto regargs       = func_props->get_str("regargs");

      auto start = SegOff_t::from_str(start_str.value());
      if(start.is_err())
        return std::format<"Expected segoff for '{}.start', got '{}'">(kv.key, *start_str);

      SegOff_t end;
      if(!start_str->empty())
      {
        auto rval = SegOff_t::from_str(*start_str);
        if(rval.is_err())
          return std::format<"Expected segoff for '{}.end', got '{}'">(kv.key, *start_str);
        end = rval.value();
      }

      CallMode mode;
      if(*mode_str == "near")
        mode = CallMode::Near;
      else if(*mode_str == "far")
        mode = CallMode::Near;
      else
        return std::format<"Unsupported mode '{}'">(*mode_str);

      std::optional<uint16_t> args = 0;
      if (auto result = std::from_chars(args_str->data(), args_str->data() + args_str->size(), *args);
          result.ec == std::errc::invalid_argument)
        return std::format<"Expected uint16_t for '{}.args', got '{}'">(kv.key, *args_str);
      if(!*args)
        args.reset();

      auto ret = type_builder.parse_type(*ret_str);
      if(ret.is_err())
        return std::format<"Expected type for '{}.ret', got '{}' | {}">(kv.key, *ret_str, ret.error());

      std::optional<OverlayRange> overlay;
      if(overlay_num && overlay_start && overlay_end)
      {
        uint16_t num = 0, start = 0, end = 0;
        if (auto result = std::from_chars(overlay_num->data(), overlay_num->data() + overlay_num->size(), num);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t for '{}.overlay_num', got '{}'">(kv.key, *overlay_num);

        if (auto result = std::from_chars(overlay_start->data(), overlay_start->data() + overlay_start->size(), num);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t for '{}.overlay_start', got '{}'">(kv.key, *overlay_start);

        if (auto result = std::from_chars(overlay_end->data(), overlay_end->data() + overlay_end->size(), num);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t for '{}.overlay_end', got '{}'">(kv.key, *overlay_end);

        overlay = OverlayRange { num, start, end };
      }
      else if (overlay_num || overlay_start || overlay_end)
        return std::format<"Overlay options only partially set for '{}'">(kv.key);

      std::vector<Register_t> registers;
      if(regargs)
      {
        size_t start = 0, end = 0;
        do
        {
          end = regargs->find(',', start);
          auto regstr = regargs->substr(start, end - start);
          if(auto reg = Register_t::from_str_upper(regstr); !reg)
            return std::format<"Failed to parse register name: {}">(regstr);
          else
            registers.emplace_back(*reg);
          start = end + 1;
        } while (end != std::string::npos);
      }

      if(!indirect)
      {
        bool dont_pop_args_val = false;
        if(dont_pop_args && *dont_pop_args == "true")
          dont_pop_args_val = true;

        funcs.emplace_back(Func { kv.key, *start, end, overlay, mode, *ret, args, registers, dont_pop_args_val });
      }
      else if(mode == CallMode::Far)
      {
        indirects.emplace_back(Indirect{ *start, *ret, (args ? *args : uint16_t(0)) });
      }
      else
        return std::format<"Cannot have an indirect near call: {}">(kv.key);
    }
    return { nullptr };
  }

  Result<std::nullptr_t, std::string> Config::parse_globals(std::shared_ptr<bsl::node_t> root)
  {
    auto topnode = root->get_node("dis86.globals");
    if(!topnode)
      return std::string("Failed to get the globals node");

    for(const auto& kv : topnode->kv_arr)
    {
      auto glob_props = kv.as_node();
      if(!glob_props)
        return std::string("Expected global properties");

      auto off_str = glob_props->get_str("off");
      if(!off_str)
        return std::format<"No global 'off' property for '{}'">(kv.key);

      auto type_str = glob_props->get_str("type");
      if(!type_str)
        return std::format<"No global 'type' property for '{}'">(kv.key);

      uint16_t off = 0;
      if (auto result = std::from_chars(off_str->data(), off_str->data() + off_str->size(), off);
          result.ec == std::errc::invalid_argument)
        return std::format<"Expected uint16_t hex for '{}.off', got '{}'">(kv.key, *off_str);

      auto typ = type_builder.parse_type(*type_str);
      if(typ.is_err()) // FIXME: Make this a hard error.. currently the configs have undefined struct names.. need to support that first :-(
      {
        std::cerr << std::format<"WRN: Expected type for '{}.type', got '{}' | {}">(kv.key, *type_str, typ.error()) << std::endl;
        typ = Type { Type::Unknown };
      }

      globals.emplace_back(Global { kv.key, off, *typ });
    }
    return { nullptr };
  }

  Result<std::nullptr_t, std::string> Config::parse_text_section(std::shared_ptr<bsl::node_t> root)
  {
    auto topnode = root->get_node("dis86.text_section");
    if(!topnode)
      return std::string("Failed to get the text_section node");

    for(const auto& kv : topnode->kv_arr)
    {
      auto ts_props = kv.as_node();
      if(!ts_props)
        return std::string("Expected text_section properties");

      auto start_str = ts_props->get_str("start");
      if(!start_str)
        return std::format<"No text_section 'start' property for '{}'">(kv.key);

      auto end_str = ts_props->get_str("end");
      if(!end_str)
        return std::format<"No text_section 'end' property for '{}'">(kv.key);

      auto type_str = ts_props->get_str("type");
      if(!type_str)
        return std::format<"No text_section 'type' property for '{}'">(kv.key);

      auto access_str = ts_props->get_str("access");

      auto start = SegOff_t::from_str(*start_str);
      if(start.is_err())
        return std::format<"Expected segoff for '{}.start', got '{}' | {}">(kv.key, *start_str, start.error());

      auto end = SegOff_t::from_str(*end_str);
      if(end.is_err())
        return std::format<"Expected segoff for '{}.end', got '{}' | {}">(kv.key, *end_str, end.error());

      auto typ = type_builder.parse_type(*type_str);
      if(typ.is_err())
        return std::format<"Expected segoff for '{}.end', got '{}'">(kv.key, *type_str, typ.error());

      std::optional<SegOff_t> access;
      if(access_str)
      {
        auto val = SegOff_t::from_str(*access_str);
        if(val.is_err())
          return std::format<"Expected segoff for '{}.access', got '{}' | {}">(kv.key, *access_str, val.error());
        access = *val;
      }

      text_section.emplace_back(TextSectionRegion { kv.key, *start, *end, *typ, access });
    }
    return { nullptr };
  }
}
