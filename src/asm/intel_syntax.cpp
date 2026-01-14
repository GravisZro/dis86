#include "intel_syntax.h"

#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <format>
#include <bit>

namespace intel_syntax
{
  using namespace instr;
  using namespace segoff;

  void format_operand(std::string& s, Instruction_t& ins, Operand_t oper)
  {
    switch(oper.type)
    {
      case Operand_t::None: break;

      case Operand_t::Reg:
        s += oper.reg.reg.name();
        break;

      case Operand_t::Mem:
        switch(oper.mem.sz)
        {
          case Size_e::Size8:  s += "BYTE PTR "; break;
          case Size_e::Size16: s += "WORD PTR "; break;
          case Size_e::Size32: s += "DWORD PTR "; break;
        }
        s += oper.mem.sreg.name();
        s += ":";

        if(!oper.mem.reg1 && !oper.mem.reg2)
        {
          if(oper.mem.off)
            s += std::format<"{:x}">(*oper.mem.off);
        }
        else
        {
          s += "[";
          if(oper.mem.reg1)
            s += oper.mem.reg1->name();
          if(oper.mem.reg2)
          {
            s.push_back('+');
            s += oper.mem.reg2->name();
          }

          if(oper.mem.off)
          {
            int16_t disp = std::bit_cast<int16_t>(*oper.mem.off);
            if (disp >= 0) { s += std::format<"+{:x}">(disp); }
            else           { s += std::format<"-{:x}">(-disp); }
          }
          s += "]";
        }
        break;

      case Operand_t::Imm:
        s += std::format<"{:x}">(oper.imm.val);
        break;

      case Operand_t::Rel:
        s += std::format<"{:x}">(ins.rel_addr(oper.rel).off);
        break;

      case Operand_t::Far:
        s += std::format<"{:x}:">(oper.far.seg) + std::format<"{:x}">(oper.far.off);
        //s += std::format<"{:x}:{:x}">(oper.far.seg, oper.far.off);
        break;
    };
  }


  void format_instr_impl(std::string& s, Instruction_t& ins, segment<uint8_t> bytes, bool with_detail)
  {
    if(with_detail)
    {
      s += std::format<"{}:\t">(ins.addr.to_str());
      for(size_t i = 0; i < bytes.size(); i++)
        s += std::format<"{:02x} ">(bytes[i]);
      size_t used = bytes.size() * 3;
      size_t remain = (used <= 21) ? 21 - used : 0;
      s += std::format<"{}\t">(remain);
      //s += std::format<"{:1$}\t">(remain);
    }

    if(ins.rep.has_value())
    {
      if(*ins.rep == Rep_e::NE)
        s += "repne ";
      else if(*ins.rep == Rep_e::EQ)
        s += "rep ";
    }

    s += std::format<"{:<5}">(instr_op_mneumonic.at(uint8_t(ins.opcode)));
    bool first = true;
    for(uint8_t i = 0; i < 3 && ins.operands[i].type != Operand_t::None; i++)
    {
      if(((1 << i) & ins.intel_hidden_operand_bitmask) != 0)
        continue;
      if(first)
      {
        s += "  ";
        first = false;
      }
      else
        s += ",";
      format_operand(s, ins, ins.operands[i]);
    }
  }

  void format_data_impl(std::string& s, SegOff_t addr, segment<uint8_t> bytes, bool with_detail)
  {
    if(with_detail)
    {
      s += std::format<"{}:\t">(addr.to_str());
      for(size_t i = 0; i < bytes.size(); i++)
        s += std::format<"{:02x} ">(bytes[i]);
      size_t used = bytes.size() * 3;
      size_t remain = (used <= 21) ? 21 - used : 0;
      s += std::format<"{}\t">(remain);
      // s += std::format<"{:1$}\t">(remain);
    }
    s += "(data)";
  }

  // FIXME: THIS IS KLUDGY
  std::string format(SegOff_t addr, std::optional<Instruction_t> ins, segment<uint8_t> bytes, bool with_detail)
  {
    std::string s;
    if(ins)
      format_instr_impl(s, *ins, bytes, with_detail);
    else
      format_data_impl(s, addr, bytes, with_detail);
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); }).base(),
            s.end());
    return s;
  }
}
