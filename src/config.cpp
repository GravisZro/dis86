#include "config.h"

#include <format>
#include <charconv>

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



  Result<Config, std::string> Config::from_path(const std::string& path)
  {
    Config cfg;
/*
    let dat = std::fs::read_to_string(path)
                  .map_err(|err| format!("Failed to read file with: {}'", err))?;

    let root = bsl::parse(&dat)
                   .ok_or_else(|| format!("Failed to parse config"))?;
    */
    std::string filedata;
    bsl::error_e error = bsl::success;
    std::shared_ptr<bsl::node_t> root = bsl::parse_new(filedata, &error);

    cfg.parse_structs(root); // Important for this to go first and build types
    cfg.parse_functions(root);
    cfg.parse_globals(root);
    cfg.parse_text_section(root);

    return cfg;
  }
  //Result<std::nullptr_t, std::string> Config::parse_functions(std::shared_ptr<bsl::node_t> root) { return std::string("not implemented"); }
  //Result<std::nullptr_t, std::string> Config::parse_structs(std::shared_ptr<bsl::node_t> root) { return std::string("not implemented"); }
  Result<std::nullptr_t, std::string> Config::parse_globals(std::shared_ptr<bsl::node_t> root) { return std::string("not implemented"); }
  Result<std::nullptr_t, std::string> Config::parse_text_section(std::shared_ptr<bsl::node_t> root) { return std::string("not implemented"); }

  Result<std::nullptr_t, std::string> Config::parse_structs(std::shared_ptr<bsl::node_t> root)
  {
    auto topnode = root->get_node("dis86.structures");
    if(!topnode)
      return std::string("Failed to get the structures node");

    for(const auto& struct_kv : topnode->kv_arr)
    {
      auto struct_props = struct_kv.as_node();
      if(!struct_props)
        return std::string("Expected structure properties");
      auto sz = struct_props->get_str("size");
      if(!sz)
        return std::format<"No function 'size' property for '{}'">(struct_kv.key);

      uint16_t size = 0;
      if (auto result = std::from_chars(sz->data(), sz->data() + sz->size(), size);
          result.ec == std::errc::invalid_argument)
        return std::format<"Expected uint16_t for '{}.start', got '{}'">(struct_kv.key, *sz);

      auto members = struct_props->get_node("members");
      if(!members)
        return std::format<"Expected {}.members node">(struct_kv.key);

      std::vector<StructMember> member_data;
      for(const auto& member_kv : members->kv_arr)
      {
        auto member_node = member_kv.as_node();
        if(!member_node)
          return std::format<"Expected member properties for {}.members.{}">(struct_kv.key, member_kv.key);

        auto off_str = member_node->get_str("off");
        if(!off_str)
          return std::format<"No 'off' property for '{}.members.{}'">(struct_kv.key, member_kv.key);

        auto type_str = member_node->get_str("type");
        if(!type_str)
          return std::format<"No 'type' property for '{}.members.{}'">(struct_kv.key, member_kv.key);

        uint16_t off = 0;
        if (auto result = std::from_chars(off_str->data(), off_str->data() + off_str->size(), off);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t hex for '{}.members.{}.off', got '{}'">(struct_kv.key, member_kv.key, *off_str);

        auto typ = type_builder.parse_type(*type_str);

        // let typ: Type = match type_str.parse() {
        //   Ok(typ) => typ,
        //   Err(err) => {
        //     // FIXME: Make this a hard error.. currently the configs have undefined struct names.. need to support that first :-(
        //     eprintln!("WRN: Expected type for '{}.members.{}.type', got '{}' | {}", name, key, type_str, err);
        //     Type::Unknown
        //   }
        // };
        if(typ.is_err())
          return typ.error();
        member_data.emplace_back(StructMember { member_kv.key, typ.value(), off });
      }
      auto s = Struct { struct_kv.key, size, member_data };
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

    for(const auto& func_kv : topnode->kv_arr)
    {
      auto func_props = func_kv.as_node();
      if(!func_props)
        return std::string("Expected function properties");

      auto start_str = func_props->get_str("start");
      if(!start_str)
        return std::format<"No function 'start' property for '{}'">(func_kv.key);

      auto end_str = func_props->get_str("end");
      if(!end_str)
        return std::format<"No function 'end' property for '{}'">(func_kv.key);

      auto mode_str = func_props->get_str("mode");
      if(!mode_str)
        return std::format<"No function 'mode' property for '{}'">(func_kv.key);

      auto ret_str = func_props->get_str("ret");
      if(!ret_str)
        return std::format<"No function 'ret' property for '{}'">(func_kv.key);

      auto args_str = func_props->get_str("args");
      if(!args_str)
        return std::format<"No function 'args' property for '{}'">(func_kv.key);

      auto dont_pop_args = func_props->get_str("dont_pop_args");
      auto indirect      = func_props->get_str("indirect_call_location");
      auto overlay_num   = func_props->get_str("overlay_num");
      auto overlay_start = func_props->get_str("overlay_start");
      auto overlay_end   = func_props->get_str("overlay_end");
      auto regargs       = func_props->get_str("regargs");

      auto start = SegOff_t::from_str(start_str.value());
      if(start.is_err())
        return std::format<"Expected segoff for '{}.start', got '{}'">(func_kv.key, *start_str);

      SegOff_t end;
      if(!start_str->empty())
      {
        auto rval = SegOff_t::from_str(*start_str);
        if(rval.is_err())
          return std::format<"Expected segoff for '{}.end', got '{}'">(func_kv.key, *start_str);
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
        return std::format<"Expected uint16_t for '{}.args', got '{}'">(func_kv.key, *args_str);
      if(!*args)
        args.reset();

      auto ret = type_builder.parse_type(*ret_str);
      if(ret.is_err())
        return std::format<"Expected type for '{}.ret', got '{}' | {}">(func_kv.key, *ret_str, ret.error());

      std::optional<OverlayRange> overlay;
      if(overlay_num && overlay_start && overlay_end)
      {
        uint16_t num = 0, start = 0, end = 0;
        if (auto result = std::from_chars(overlay_num->data(), overlay_num->data() + overlay_num->size(), num);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t for '{}.overlay_num', got '{}'">(func_kv.key, *overlay_num);

        if (auto result = std::from_chars(overlay_start->data(), overlay_start->data() + overlay_start->size(), num);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t for '{}.overlay_start', got '{}'">(func_kv.key, *overlay_start);

        if (auto result = std::from_chars(overlay_end->data(), overlay_end->data() + overlay_end->size(), num);
            result.ec == std::errc::invalid_argument)
          return std::format<"Expected uint16_t for '{}.overlay_end', got '{}'">(func_kv.key, *overlay_end);

        overlay = OverlayRange { num, start, end };
      }
      else if (overlay_num || overlay_start || overlay_end)
        return std::format<"Overlay options only partially set for '{}'">(func_kv.key);

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

        funcs.emplace_back(Func { func_kv.key, *start, end, overlay, mode, *ret, args, registers, dont_pop_args_val });
      }
      else if(mode == CallMode::Far)
      {
        indirects.emplace_back(Indirect{ *start, *ret, (args ? *args : uint16_t(0)) });
      }
      else
        return std::format<"Cannot have an indirect near call: {}">(func_kv.key);
    }
    return { nullptr };
  }
#if 0
    Result<std::nullptr_t, std::string> parse_globals(std::shared_ptr<bsl::node_t> root)
    {
      let glob = root.get_node("dis86.globals")
                     .ok_or_else(|| format!("Failed to get the globals node"))?;

      for (key, val) in glob.iter() {
          let g = val.as_node()
                      .ok_or_else(|| format!("Expected global properties"))?;

          let off_str = g.get_str("off")
                            .ok_or_else(|| format!("No global 'off' property for '{}'", key))?;
          let type_str = g.get_str("type")
                             .ok_or_else(|| format!("No global 'type' property for '{}'", key))?;

          let off = parse_uint16_t(off_str)
                        .map_err(|_| format!("Expected uint16_t hex for '{}.off', got '{}'", key, off_str))?;
          let typ = match self.type_builder.parse_type(type_str) {
                                                                 Ok(typ) => typ,
                                                                 Err(err) => {
                                                                               // FIXME: Make this a hard error.. currently the configs have undefined struct names.. need to support that first :-(
                                                                               eprintln!("WRN: Expected type for '{}.type', got '{}' | {}", key, type_str, err);
          Type::Unknown
        }
    };

    self.globals.push(Global {
      name: key.to_string(),
      offset: off,
      typ,
    });
    }
    Ok(())
    }


  Result<std::nullptr_t, std::string> parse_text_section(std::shared_ptr<bsl::node_t> root)
  {
    let func = root.get_node("dis86.text_section")
                   .ok_or_else(|| format!("Failed to get the text_section node"))?;

    for (key, val) in func.iter() {
        let f = val.as_node()
                    .ok_or_else(|| format!("Expected text_section properties"))?;

        let start_str = f.get_str("start")
                            .ok_or_else(|| format!("No text_section 'start' property for '{}'", key))?;
        let end_str = f.get_str("end")
                          .ok_or_else(|| format!("No text_section 'end' property for '{}'", key))?;
        let type_str = f.get_str("type")
                           .ok_or_else(|| format!("No text_section 'type' property for '{}'", key))?;
        let access_str = f.get_str("access");

        let start: SegOff_t = start_str.parse()
                                   .map_err(|_| format!("Expected segoff for '{}.start', got '{}'", key, start_str))?;
        let end: SegOff_t = end_str.parse()
                                 .map_err(|_| format!("Expected segoff for '{}.end', got '{}'", key, end_str))?;
        let typ: Type = self.type_builder.parse_type(type_str)
                             .map_err(|err| format!("Expected type for '{}.type', got '{}' | {}", key, type_str, err))?;
        let access: std::optional<SegOff_t> = match access_str {
                                                                None => None,
                                                                Some(access) => Some(access.parse()
                                                                                          .map_err(|err| format!("Expected segoff for '{}.access', got '{}' | {}", key, access, err))?),
                                                                    };

        self.text_section.push(TextSectionRegion {
          name: key.to_string(),
          start,
          end,
          typ,
          access,
        });
      }

    Ok(())
  }
#endif
}
