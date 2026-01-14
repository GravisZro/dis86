#include "instr.h"

#include <cassert>

namespace instr
{
  Register_t::e Register_t::reg8(uint8_t num)
  {
    assert(num < 8);
    return static_cast<e>(static_cast<uint8_t>(AL) + num);
  }

  Register_t::e Register_t::reg16(uint8_t num)
  {
    assert(num < 8);
    return static_cast<e>(static_cast<uint8_t>(AX) + num);
  }

  Register_t::e Register_t::sreg16(uint8_t num)
  {
    assert(num < 4);
    return static_cast<e>(static_cast<uint8_t>(ES) + num);
  }

  std::string_view Register_t::name(void) const
  {
    assert(value < info.size());
    return info[value].name;
  }

  std::optional<Register_t::e> Register_t::from_str_upper(const std::string& str)
  {
    static const std::map<std::string_view, e> names =
      {
        {
          { "AX", AX },
          { "CX", CX },
          { "DX", DX },
          { "BX", BX },
          { "SP", SP },
          { "BP", BP },
          { "SI", SI },
          { "DI", DI },
          { "AL", AL },
          { "CL", CL },
          { "DL", DL },
          { "BL", BL },
          { "AH", AH },
          { "CH", CH },
          { "DH", DH },
          { "BH", BH },
          { "ES", ES },
          { "CS", CS },
          { "SS", SS },
          { "DS", DS },
          { "IP", IP },
          { "FLAGS", FLAGS },
        }
      };
    auto pos = names.find(str);
    if(pos == std::end(names))
      return {};
    else
      return pos->second;
  }


  const std::array<RegInfo_t, 22> info =
    {
      {
        { "ax",    Size_e::Size16, false },
        { "cx",    Size_e::Size16, false },
        { "dx",    Size_e::Size16, false },
        { "bx",    Size_e::Size16, false },
        { "sp",    Size_e::Size16, false },
        { "bp",    Size_e::Size16, false },
        { "si",    Size_e::Size16, false },
        { "di",    Size_e::Size16, false },
        { "al",    Size_e::Size8,  false },
        { "cl",    Size_e::Size8,  false },
        { "dl",    Size_e::Size8,  false },
        { "bl",    Size_e::Size8,  false },
        { "ah",    Size_e::Size8,  false },
        { "ch",    Size_e::Size8,  false },
        { "dh",    Size_e::Size8,  false },
        { "bh",    Size_e::Size8,  false },
        { "es",    Size_e::Size16, true  },
        { "cs",    Size_e::Size16, true  },
        { "ss",    Size_e::Size16, true  },
        { "ds",    Size_e::Size16, true  },
        { "ip",    Size_e::Size16, false },
        { "flags", Size_e::Size16, false },
      }
    };

  std::string_view to_str(Size_e val)
  {
    switch(val)
    {
      case Size_e::Size8: return "Size_e::Size8";
      case Size_e::Size16: return "Size_e::Size16";
      case Size_e::Size32: return "Size_e::Size32";
      default:  return "Enumeration value out of range";
    }
  }

  std::string Instruction_t::to_str(void) const
  {
    assert(false); // TODO
    return "";
  }
}
