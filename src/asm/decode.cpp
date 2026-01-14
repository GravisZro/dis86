#include "decode.h"

#include "instr_fmt.h"

#include <format>

namespace decode
{
  using namespace instr;
  using namespace region;
  using namespace instr_fmt;


  Result<Operand_t, std::string> operand_reg(Register_t r)
  { return { OperandReg_t{ r } }; }

  Result<Operand_t, std::string> operand_imm8(uint8_t imm)
  { return { OperandImm_t{ Size_e::Size8, imm } }; }

  Result<Operand_t, std::string> operand_imm16(uint16_t imm)
  { return { OperandImm_t{ Size_e::Size16, imm } }; }

  Result<Operand_t, std::string> operand_imm16(uint16_t imm, Size_e sz)
  { return { OperandImm_t{ Size_e::Size16, imm } }; }

  Result<Operand_t, std::string> operand_rel(RegionIter_t& bin, Size_e sz)
  {

    if(sz == Size_e::Size8)
    {
      if(auto v = bin.fetch_sext(); v.is_ok())
        return { OperandRel_t { *v } };
      else
        return v.error();
    }
    if(sz == Size_e::Size16)
    {
      if(auto v = bin.fetch_u16(); v.is_ok())
        return { OperandRel_t { *v } };
      else
        return v.error();
    }
    return std::format<"Invalid relative addressing size: {}">(to_str(sz));
  }


  Result<Operand_t, std::string> operand_src(Size_e sz)
  {
    return Operand_t { OperandMem_t { sz,
        Register_t::DS, // TODO FIMXE: ARE SEG OVERRIDES ALLOWED FOR THESE??
        Register_t::SI,
        {},
        {}
    } };
  }

  Result<Operand_t, std::string> operand_dst(Size_e sz)
  {
    return Operand_t { OperandMem_t { sz,
        Register_t::ES, // TODO FIMXE: ARE SEG OVERRIDES ALLOWED FOR THESE??
        Register_t::DI,
        {},
        {}
    } };
  }

  Result<Operand_t, std::string> operand_far(RegionIter_t& bin)
  {
    auto off = bin.fetch_u16();
    if(off.is_err())
      return off.error();
    auto seg = bin.fetch_u16();
    if(seg.is_err())
      return seg.error();
    return Operand_t { OperandFar_t { *seg, *off } };
  }

  Result<Operand_t, std::string> operand_moff(RegionIter_t& bin, Size_e sz, std::optional<Register_t> prefix_sreg)
  {
    if(auto v = bin.fetch_u16(); v.is_err())
      return v.error();
    else
      return Operand_t { OperandMem_t { sz,
          prefix_sreg.value_or(Register_t::DS),
          {},
          {},
          *v
      } };
  }

  Result<Operand_t, std::string> operand_rm(RegionIter_t& bin, Size_e sz,  uint8_t modrm, std::optional<Register_t> prefix_sreg)
  {
    uint8_t mode = modrm_mode(modrm);
    uint8_t rm = modrm_rm(modrm);
    if (mode == 3) // Register mode
    {
      if(sz == Size_e::Size8) { return operand_reg(Register_t::reg8(rm)); }
      else if(sz == Size_e::Size16) { return operand_reg(Register_t::reg16(rm)); }
      else { return std::string("Only 8-bit and 16-bit registers are allowed"); }
    }
    else if(mode == 0 && rm == 6) // Direct addressing mode: 16-bit
      return operand_moff(bin, sz, prefix_sreg);

    // Everything else uses some indirect register mode
    OperandMem_t rval;
    rval.sz = sz;

    switch(rm)
    {
      case 0: rval.sreg = Register_t::DS; rval.reg1 = Register_t::BX; rval.reg2 = Register_t::SI; break;
      case 1: rval.sreg = Register_t::DS; rval.reg1 = Register_t::BX; rval.reg2 = Register_t::DI; break;
      case 2: rval.sreg = Register_t::SS; rval.reg1 = Register_t::BP; rval.reg2 = Register_t::SI; break;
      case 3: rval.sreg = Register_t::SS; rval.reg1 = Register_t::BP; rval.reg2 = Register_t::DI; break;
      case 4: rval.sreg = Register_t::DS; rval.reg1 = Register_t::SI; break;
      case 5: rval.sreg = Register_t::DS; rval.reg1 = Register_t::DI; break;
      case 6: rval.sreg = Register_t::SS; rval.reg1 = Register_t::BP; break;
      case 7: rval.sreg = Register_t::DS; rval.reg1 = Register_t::BX; break;
    }

    // Handle immediate dispacements
    if(mode == 1)
    {
      if(auto v = bin.fetch_sext(); v.is_err())
        return v.error();
      else
        rval.off = *v;
    }
    else if(mode == 2)
    {
      if(auto v = bin.fetch_u16(); v.is_err())
        return v.error();
      else
        rval.off = *v;
    }

    if(prefix_sreg)
      rval.sreg = *prefix_sreg;

    return { rval };
  }

  Result<Operand_t, std::string> operand_rm8(RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg)
  { return operand_rm(bin, Size_e::Size8, modrm, sreg); }

  Result<Operand_t, std::string> operand_rm16(RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg)
  { return operand_rm(bin, Size_e::Size16, modrm, sreg); }


  Result<Operand_t, std::string> operand_m8(RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg)
  {
    auto oper = operand_rm(bin, Size_e::Size8, modrm, sreg);
    if(oper.is_ok_and<bool>([](Operand_t& oper) { return oper == Operand_t::Mem; }, false))
      return std::string("Register used where memory operand was required");
    return oper;
  }

  Result<Operand_t, std::string> operand_m16(RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg)
  {
    auto oper = operand_rm(bin, Size_e::Size16, modrm, sreg);
    if(oper.is_ok_and<bool>([](Operand_t& oper) { return oper == Operand_t::Mem; }, false))
      return std::string("Register used where memory operand was required");
    return oper;
  }

  Result<Operand_t, std::string> operand_m32(RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg)
  {
    auto oper = operand_rm(bin, Size_e::Size32, modrm, sreg);
    if(oper.is_ok_and<bool>([](Operand_t& oper) { return oper == Operand_t::Mem; }, false))
      return std::string("Register used where memory operand was required");
    return oper;
  }

  Result<std::optional<std::pair<Instruction_t, segment<uint8_t>>>, std::string> decode_one_impl(RegionIter_t& bin)
  {
    SegOff_t start_addr = bin.addr();
    if(start_addr == bin.end_addr()) {
      return std::optional<std::pair<Instruction_t, segment<uint8_t>>>{};
    }

    // First parse any prefixes
    Instruction_t ins;
    ins.addr = start_addr;

    std::optional<uint8_t> sreg;

    for(bool exit = false; !exit; )
    {
      switch(bin.peek())
      {
        case 0x26: sreg = Register_t::ES; break;
        case 0x2e: sreg = Register_t::CS; break;
        case 0x36: sreg = Register_t::SS; break;
        case 0x3e: sreg = Register_t::DS; break;
        case 0xf2: ins.rep = Rep_e::NE; break;
        case 0xf3: ins.rep = Rep_e::EQ; break;
        default: exit = true; break;
      }
      if(!exit)
        bin.advance();
    }

    // Now parse the main level1 opcode
    Result<uint8_t, std::string> opcode1 = bin.fetch();
    std::optional<uint8_t> opcode2;

    if(opcode1.is_err())
      return opcode1.error();

    auto ret = lookup(*opcode1, opcode2);

    // Need a level 2 opcode to do the lookup?
    if(ret.error() == Error::NeedOpcode2)
    {
      auto b = bin.peek_checked();
      if(b.is_err())
        return b.error();
      opcode2 = modrm_op2(*b);
      ret = lookup(*opcode1, opcode2);
    }
    else if(ret.error() == Error::NeedOpcode2Ext0F)
    {
      auto b = bin.peek_checked();
      if(b.is_err())
        return b.error();
      bin.advance();
      opcode2 = *b;
      ret = lookup(*opcode1, opcode2);
    }

    if(ret.is_err())
    {
      uint8_t op1_val = *opcode1;
      uint8_t op2_val = *opcode2;
      return std::format<"Failed to find instruction fmt for opcode1={:02x}">(op1_val) +
             std::format<", opcode2={:02x} at ">(op2_val) +
             start_addr.to_str();
      /*
      return std::format<"Failed to find instruction fmt for opcode1={:02x}, opcode2={:02x} at {}">(
          *opcode1,
          *opcode2,
          start_addr.to_str().c_str());
*/
    }

    // Unpack
    instruction_format_t& fmt = *ret;

    if(fmt.op == operation_e::INVAL)
      return std::format<"Unsupported or invalid instruction at {}">(start_addr.to_str());

    ins.opcode = fmt.op;
    ins.intel_hidden_operand_bitmask = fmt.hidden;

    // Do we need a modrm?
    uint8_t modrm = 0;
    if(fmt.requires_modrm())
    {
      if(auto v = bin.fetch(); v.is_err())
        return v.error();
      else
        modrm = *v;
    }

    // Process the format and build up the instruction
    for(uint8_t i = 0; i < 3; ++i)
    {
      Result<Operand_t, std::string> res = std::string();
      switch(fmt.operands[i])
      {
        // Sentinel
        case operand_e::NONE: break;

          // Implied 16-bit register operands
        case operand_e::AX: res = operand_reg(Register_t::AX); break;
        case operand_e::CX: res = operand_reg(Register_t::CX); break;
        case operand_e::DX: res = operand_reg(Register_t::DX); break;
        case operand_e::BX: res = operand_reg(Register_t::BX); break;
        case operand_e::SP: res = operand_reg(Register_t::SP); break;
        case operand_e::BP: res = operand_reg(Register_t::BP); break;
        case operand_e::SI: res = operand_reg(Register_t::SI); break;
        case operand_e::DI: res = operand_reg(Register_t::DI); break;

          // Implied 8-bit register operands
        case operand_e::AL: res = operand_reg(Register_t::AL); break;
        case operand_e::CL: res = operand_reg(Register_t::CL); break;
        case operand_e::DL: res = operand_reg(Register_t::DL); break;
        case operand_e::BL: res = operand_reg(Register_t::BL); break;
        case operand_e::AH: res = operand_reg(Register_t::AH); break;
        case operand_e::CH: res = operand_reg(Register_t::CH); break;
        case operand_e::DH: res = operand_reg(Register_t::DH); break;
        case operand_e::BH: res = operand_reg(Register_t::BH); break;

          // Implied segment register operands
        case operand_e::ES: res = operand_reg(Register_t::ES); break;
        case operand_e::CS: res = operand_reg(Register_t::CS); break;
        case operand_e::SS: res = operand_reg(Register_t::SS); break;
        case operand_e::DS: res = operand_reg(Register_t::DS); break;

          // Implied segment register operands
        case operand_e::FLAGS: res = operand_reg(Register_t::FLAGS); break;
        case operand_e::LIT1:  res = operand_imm8(1); break;
        case operand_e::LIT3:  res = operand_imm8(3); break;

          // Implied string operations operands
        case operand_e::SRC8:  res = operand_src(Size_e::Size8); break;
        case operand_e::SRC16: res = operand_src(Size_e::Size8); break;
        case operand_e::DST8:  res = operand_dst(Size_e::Size8); break;
        case operand_e::DST16: res = operand_dst(Size_e::Size8); break;

          // Explicit register operands
        case operand_e::R8:   res = operand_reg(Register_t::reg8  (modrm_reg(modrm))); break;
        case operand_e::R16:  res = operand_reg(Register_t::reg16 (modrm_reg(modrm))); break;
        case operand_e::SREG: res = operand_reg(Register_t::sreg16(modrm_reg(modrm))); break;

          // Explicit memory operands
        case operand_e::M8:   res = operand_m8 (bin, modrm, sreg); break;
        case operand_e::M16:  res = operand_m16(bin, modrm, sreg); break;
        case operand_e::M32:  res = operand_m32(bin, modrm, sreg); break;

          // Explicit register or memory operands (modrm)
        case operand_e::RM8:  res = operand_rm8 (bin, modrm, sreg); break;
        case operand_e::RM16: res = operand_rm16(bin, modrm, sreg); break;

          // Explicit immediate data
        case operand_e::IMM8:
          if(auto v = bin.fetch(); v.is_err())
            return v.error();
          else
            res = operand_imm8(*v);
          break;

        case operand_e::IMM8_EXT:
          if(auto v = bin.fetch_sext(); v.is_err())
            return v.error();
          else
            res = operand_imm16(*v);
          break;

        case operand_e::IMM16:
          if(auto v = bin.fetch_u16(); v.is_err())
            return v.error();
          else
            res = operand_imm16(*v);
          break;

          // Explicit far32 jump immediate
        case operand_e::FAR32: res = operand_far(bin); break;

          // Explicit 16-bit immediate used as a memory offset into DS
        case operand_e::MOFF8:  res = operand_moff(bin, Size_e::Size8 , sreg); break;
        case operand_e::MOFF16: res = operand_moff(bin, Size_e::Size16, sreg); break;

          // Explicit relative offsets (branching / calls)
        case operand_e::REL8:   res = operand_rel(bin, Size_e::Size8); break;
        case operand_e::REL16:  res = operand_rel(bin, Size_e::Size16); break;
      }

      if(fmt.operands[i] != operand_e::NONE)
      {
        if(res.is_err())
          return res.error();
        ins.operands[i] = *res;
      }
    }

    ins.n_bytes = start_addr.offset_to(bin.addr());

    return { std::make_pair(ins, bin.slice(start_addr, ins.n_bytes)) };
  }

  Result<std::optional<std::pair<Instruction_t, segment<uint8_t>>>, std::string> decode_one(RegionIter_t& bin)
  {
    SegOff_t save_addr = bin.addr();
    auto rval = decode_one_impl(bin);
    if(rval.is_err())
      bin.reset_addr(save_addr);
    return rval;
  }
}

#if ENABLE_TESTS
#include <vector>
#include <iostream>
#include <unistd.h>
#include "common/segment.h"
#include "intel_syntax.h"

namespace decode
{
  void test(void)
  {
    struct TestCase_t
    {
      size_t addr;
      std::vector<uint8_t> dat;
      std::string str;
    };

    const std::vector<TestCase_t> tests =
    {
      { 0x0000, { 0xba, 0xa7, 0x0e },             "mov    dx,0xea7" },
      { 0x0008, { 0xb4, 0x30 },                   "mov    ah,0x30" },
      { 0x000a, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x000c, { 0x8b, 0x2e, 0x02, 0x00 },       "mov    bp,WORD PTR ds:0x2" },
      { 0x0010, { 0x8b, 0x1e, 0x2c, 0x00 },       "mov    bx,WORD PTR ds:0x2c" },
      { 0x0016, { 0xa3, 0x7d, 0x00 },             "mov    WORD PTR ds:0x7d,ax" },
      { 0x0019, { 0x8c, 0x06, 0x7b, 0x00 },       "mov    WORD PTR ds:0x7b,es" },
      { 0x001d, { 0x89, 0x1e, 0x77, 0x00 },       "mov    WORD PTR ds:0x77,bx" },
      { 0x0021, { 0x89, 0x2e, 0x91, 0x00 },       "mov    WORD PTR ds:0x91,bp" },
      { 0x0025, { 0xe8, 0x52, 0x01 },             "call   0x17a" },
      { 0x0028, { 0xc4, 0x3e, 0x75, 0x00 },       "les    di,DWORD PTR ds:0x75" },
      { 0x002c, { 0x8b, 0xc7 },                   "mov    ax,di" },
      { 0x002e, { 0x8b, 0xd8 },                   "mov    bx,ax" },
      { 0x0030, { 0xb9, 0xff, 0x7f },             "mov    cx,0x7fff" },
      { 0x0033, { 0xfc },                         "cld" },
      { 0x0003, { 0x2e, 0x89, 0x16, 0x60, 0x02 }, "mov    WORD PTR cs:0x260,dx" },
      { 0x0014, { 0x8e, 0xda },                   "mov    ds,dx" },
      { 0x0036, { 0xe3, 0x43 },                   "jcxz   0x7b" },
      { 0x0038, { 0x43 },                         "inc    bx" },
      { 0x0039, { 0x26, 0x38, 0x05 },             "cmp    BYTE PTR es:[di],al" },
      { 0x003c, { 0x75, 0xf6 },                   "jne    0x34" },
      { 0x003e, { 0x80, 0xcd, 0x80 },             "or     ch,0x80" },
      { 0x0041, { 0xf7, 0xd9 },                   "neg    cx" },
      { 0x0043, { 0x89, 0x0e, 0x75, 0x00 },       "mov    WORD PTR ds:0x75,cx" },
      { 0x0047, { 0xb9, 0x02, 0x00 },             "mov    cx,0x2" },
      { 0x004a, { 0xd3, 0xe3 },                   "shl    bx,cl" },
      { 0x004c, { 0x83, 0xc3, 0x10 },             "add    bx,0x10" },
      { 0x004f, { 0x83, 0xe3, 0xf0 },             "and    bx,0xfff0" },
      { 0x0052, { 0x89, 0x1e, 0x79, 0x00 },       "mov    WORD PTR ds:0x79,bx" },
      { 0x0056, { 0x8c, 0xd2 },                   "mov    dx,ss" },
      { 0x0058, { 0x2b, 0xea },                   "sub    bp,dx" },
      { 0x005a, { 0xbf, 0xa7, 0x0e },             "mov    di,0xea7" },
      { 0x005d, { 0x8e, 0xc7 },                   "mov    es,di" },
      { 0x005f, { 0x26, 0x8b, 0x3e, 0x2e, 0x44 }, "mov    di,WORD PTR es:0x442e" },
      { 0x0064, { 0x81, 0xff, 0x00, 0x02 },       "cmp    di,0x200" },
      { 0x0068, { 0x73, 0x08 },                   "jae    0x72" },
      { 0x006a, { 0xbf, 0x00, 0x02 },             "mov    di,0x200" },
      { 0x006d, { 0x26, 0x89, 0x3e, 0x2e, 0x44 }, "mov    WORD PTR es:0x442e,di" },
      { 0x0072, { 0xb1, 0x04 },                   "mov    cl,0x4" },
      { 0x0074, { 0xd3, 0xef },                   "shr    di,cl" },
      { 0x0076, { 0x47 },                         "inc    di" },
      { 0x0077, { 0x3b, 0xef },                   "cmp    bp,di" },
      { 0x0079, { 0x73, 0x03 },                   "jae    0x7e" },
      { 0x007b, { 0xe9, 0xcb, 0x01 },             "jmp    0x249" },
      { 0x007e, { 0x8b, 0xdf },                   "mov    bx,di" },
      { 0x0080, { 0x03, 0xda },                   "add    bx,dx" },
      { 0x0082, { 0x89, 0x1e, 0x89, 0x00 },       "mov    WORD PTR ds:0x89,bx" },
      { 0x0086, { 0x89, 0x1e, 0x8d, 0x00 },       "mov    WORD PTR ds:0x8d,bx" },
      { 0x008a, { 0xa1, 0x7b, 0x00 },             "mov    ax,WORD PTR ds:0x7b" },
      { 0x008d, { 0x2b, 0xd8 },                   "sub    bx,ax" },
      { 0x008f, { 0x8e, 0xc0 },                   "mov    es,ax" },
      { 0x0091, { 0xb4, 0x4a },                   "mov    ah,0x4a" },
      { 0x0093, { 0x57 },                         "push   di" },
      { 0x0094, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x0096, { 0x5f },                         "pop    di" },
      { 0x0097, { 0xd3, 0xe7 },                   "shl    di,cl" },
      { 0x0099, { 0xfa },                         "cli" },
      { 0x009a, { 0x8e, 0xd2 },                   "mov    ss,dx" },
      { 0x009c, { 0x8b, 0xe7 },                   "mov    sp,di" },
      { 0x009e, { 0xfb },                         "sti" },
      { 0x009f, { 0xb8, 0xa7, 0x0e },             "mov    ax,0xea7" },
      { 0x00a2, { 0x8e, 0xc0 },                   "mov    es,ax" },
      { 0x00a4, { 0x26, 0x89, 0x3e, 0x2e, 0x44 }, "mov    WORD PTR es:0x442e,di" },
      { 0x00a9, { 0x33, 0xc0 },                   "xor    ax,ax" },
      { 0x00ab, { 0x2e, 0x8e, 0x06, 0x60, 0x02 }, "mov    es,WORD PTR cs:0x260" },
      { 0x00b0, { 0xbf, 0x52, 0x45 },             "mov    di,0x4552" },
      { 0x00b3, { 0xb9, 0x04, 0xbd },             "mov    cx,0xbd04" },
      { 0x00b6, { 0x2b, 0xcf },                   "sub    cx,di" },
      { 0x00b8, { 0xfc },                         "cld" },
      { 0x00bb, { 0x83, 0x3e, 0xa0, 0x43, 0x14 }, "cmp    WORD PTR ds:0x43a0,0x14" },
      { 0x00c0, { 0x76, 0x47 },                   "jbe    0x109" },
      { 0x00c2, { 0x80, 0x3e, 0x7d, 0x00, 0x03 }, "cmp    BYTE PTR ds:0x7d,0x3" },
      { 0x00c7, { 0x72, 0x40 },                   "jb     0x109" },
      { 0x00c9, { 0x77, 0x07 },                   "ja     0xd2" },
      { 0x00cb, { 0x80, 0x3e, 0x7e, 0x00, 0x1e }, "cmp    BYTE PTR ds:0x7e,0x1e" },
      { 0x00d0, { 0x72, 0x37 },                   "jb     0x109" },
      { 0x00d2, { 0xb8, 0x01, 0x58 },             "mov    ax,0x5801" },
      { 0x00d5, { 0xbb, 0x02, 0x00 },             "mov    bx,0x2" },
      { 0x00d8, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x00da, { 0x72, 0x2a },                   "jb     0x106" },
      { 0x00dc, { 0xb4, 0x67 },                   "mov    ah,0x67" },
      { 0x00de, { 0x8b, 0x1e, 0xa0, 0x43 },       "mov    bx,WORD PTR ds:0x43a0" },
      { 0x00e2, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x00e4, { 0x72, 0x20 },                   "jb     0x106" },
      { 0x00e6, { 0xb4, 0x48 },                   "mov    ah,0x48" },
      { 0x00e8, { 0xbb, 0x01, 0x00 },             "mov    bx,0x1" },
      { 0x00eb, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x00ed, { 0x72, 0x17 },                   "jb     0x106" },
      { 0x00ef, { 0x40 },                         "inc    ax" },
      { 0x00f0, { 0xa3, 0x91, 0x00 },             "mov    WORD PTR ds:0x91,ax" },
      { 0x00f3, { 0x48 },                         "dec    ax" },
      { 0x00f4, { 0x8e, 0xc0 },                   "mov    es,ax" },
      { 0x00f6, { 0xb4, 0x49 },                   "mov    ah,0x49" },
      { 0x00f8, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x00fa, { 0x72, 0x0a },                   "jb     0x106" },
      { 0x00fc, { 0xb8, 0x01, 0x58 },             "mov    ax,0x5801" },
      { 0x00ff, { 0xbb, 0x00, 0x00 },             "mov    bx,0x0" },
      { 0x0102, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x0104, { 0x73, 0x03 },                   "jae    0x109" },
      { 0x0106, { 0xe9, 0x40, 0x01 },             "jmp    0x249" },
      { 0x0109, { 0xb4, 0x00 },                   "mov    ah,0x0" },
      { 0x010b, { 0xcd, 0x1a },                   "int    0x1a" },
      { 0x010d, { 0x89, 0x16, 0x81, 0x00 },       "mov    WORD PTR ds:0x81,dx" },
      { 0x0111, { 0x89, 0x0e, 0x83, 0x00 },       "mov    WORD PTR ds:0x83,cx" },
      { 0x0115, { 0x0a, 0xc0 },                   "or     al,al" },
      { 0x0117, { 0x74, 0x0c },                   "je     0x125" },
      { 0x0119, { 0xb8, 0x40, 0x00 },             "mov    ax,0x40" },
      { 0x011c, { 0x8e, 0xc0 },                   "mov    es,ax" },
      { 0x011e, { 0xbb, 0x70, 0x00 },             "mov    bx,0x70" },
      { 0x0121, { 0x26, 0xc6, 0x07, 0x01 },       "mov    BYTE PTR es:[bx],0x1" },
      { 0x0125, { 0x33, 0xed },                   "xor    bp,bp" },
      { 0x0127, { 0x2e, 0x8e, 0x06, 0x60, 0x02 }, "mov    es,WORD PTR cs:0x260" },
      { 0x012c, { 0xbe, 0x2e, 0x45 },             "mov    si,0x452e" },
      { 0x012f, { 0xbf, 0x4c, 0x45 },             "mov    di,0x454c" },
      { 0x0132, { 0xe8, 0xb5, 0x00 },             "call   0x1ea" },
      { 0x0135, { 0xff, 0x36, 0x73, 0x00 },       "push   WORD PTR ds:0x73" },
      { 0x0139, { 0xff, 0x36, 0x71, 0x00 },       "push   WORD PTR ds:0x71" },
      { 0x013d, { 0xff, 0x36, 0x6f, 0x00 },       "push   WORD PTR ds:0x6f" },
      { 0x0141, { 0xff, 0x36, 0x6d, 0x00 },       "push   WORD PTR ds:0x6d" },
      { 0x0145, { 0xff, 0x36, 0x6b, 0x00 },       "push   WORD PTR ds:0x6b" },
      { 0x0149, { 0x9a, 0x38, 0x0b, 0xe0, 0x02 }, "callf  0x2e0:0xb38" },
      { 0x014e, { 0x50 },                         "push   ax" },
      { 0x014f, { 0x90 },                         "nop" },
      { 0x0150, { 0x0e },                         "push   cs" },
      { 0x0151, { 0xe8, 0xca, 0x01 },             "call   0x31e" },
      { 0x0154, { 0x2e, 0x8e, 0x06, 0x60, 0x02 }, "mov    es,WORD PTR cs:0x260" },
      { 0x0159, { 0x56 },                         "push   si" },
      { 0x015a, { 0x57 },                         "push   di" },
      { 0x015b, { 0xbe, 0x4c, 0x45 },             "mov    si,0x454c" },
      { 0x015e, { 0xbf, 0x52, 0x45 },             "mov    di,0x4552" },
      { 0x0161, { 0xe8, 0x86, 0x00 },             "call   0x1ea" },
      { 0x0164, { 0x5f },                         "pop    di" },
      { 0x0165, { 0x5e },                         "pop    si" },
      { 0x0166, { 0xcb },                         "retf" },
      { 0x0167, { 0xcb },                         "retf" },
      { 0x0168, { 0x8b, 0xec },                   "mov    bp,sp" },
      { 0x016a, { 0xb4, 0x4c },                   "mov    ah,0x4c" },
      { 0x016c, { 0x8a, 0x46, 0x04 },             "mov    al,BYTE PTR ss:[bp+0x4]" },
      { 0x016f, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x0171, { 0xb9, 0x0e, 0x00 },             "mov    cx,0xe" },
      { 0x0174, { 0xba, 0x2f, 0x00 },             "mov    dx,0x2f" },
      { 0x0177, { 0xe9, 0xd5, 0x00 },             "jmp    0x24f" },
      { 0x017a, { 0x1e },                         "push   ds" },
      { 0x017b, { 0xb8, 0x00, 0x35 },             "mov    ax,0x3500" },
      { 0x017e, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x0180, { 0x89, 0x1e, 0x5b, 0x00 },       "mov    WORD PTR ds:0x5b,bx" },
      { 0x0184, { 0x8c, 0x06, 0x5d, 0x00 },       "mov    WORD PTR ds:0x5d,es" },
      { 0x0188, { 0xb8, 0x04, 0x35 },             "mov    ax,0x3504" },
      { 0x018b, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x018d, { 0x89, 0x1e, 0x5f, 0x00 },       "mov    WORD PTR ds:0x5f,bx" },
      { 0x0191, { 0x8c, 0x06, 0x61, 0x00 },       "mov    WORD PTR ds:0x61,es" },
      { 0x0195, { 0xb8, 0x05, 0x35 },             "mov    ax,0x3505" },
      { 0x0198, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x019a, { 0x89, 0x1e, 0x63, 0x00 },       "mov    WORD PTR ds:0x63,bx" },
      { 0x019e, { 0x8c, 0x06, 0x65, 0x00 },       "mov    WORD PTR ds:0x65,es" },
      { 0x01a2, { 0xb8, 0x06, 0x35 },             "mov    ax,0x3506" },
      { 0x01a5, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x01a7, { 0x89, 0x1e, 0x67, 0x00 },       "mov    WORD PTR ds:0x67,bx" },
      { 0x01ab, { 0x8c, 0x06, 0x69, 0x00 },       "mov    WORD PTR ds:0x69,es" },
      { 0x01af, { 0xb8, 0x00, 0x25 },             "mov    ax,0x2500" },
      { 0x01b2, { 0x8c, 0xca },                   "mov    dx,cs" },
      { 0x01b4, { 0x8e, 0xda },                   "mov    ds,dx" },
      { 0x01b6, { 0xba, 0x71, 0x01 },             "mov    dx,0x171" },
      { 0x01b9, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x01bb, { 0x1f },                         "pop    ds" },
      { 0x01bc, { 0xc3 },                         "ret" },
      { 0x01bd, { 0x1e },                         "push   ds" },
      { 0x01be, { 0xb8, 0x00, 0x25 },             "mov    ax,0x2500" },
      { 0x01c1, { 0xc5, 0x16, 0x5b, 0x00 },       "lds    dx,DWORD PTR ds:0x5b" },
      { 0x01c5, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x01c7, { 0x1f },                         "pop    ds" },
      { 0x01c8, { 0x1e },                         "push   ds" },
      { 0x01c9, { 0xb8, 0x04, 0x25 },             "mov    ax,0x2504" },
      { 0x01cc, { 0xc5, 0x16, 0x5f, 0x00 },       "lds    dx,DWORD PTR ds:0x5f" },
      { 0x01d0, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x01d2, { 0x1f },                         "pop    ds" },
      { 0x01d3, { 0x1e },                         "push   ds" },
      { 0x01d4, { 0xb8, 0x05, 0x25 },             "mov    ax,0x2505" },
      { 0x01d7, { 0xc5, 0x16, 0x63, 0x00 },       "lds    dx,DWORD PTR ds:0x63" },
      { 0x01db, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x01dd, { 0x1f },                         "pop    ds" },
      { 0x01de, { 0x1e },                         "push   ds" },
      { 0x01df, { 0xb8, 0x06, 0x25 },             "mov    ax,0x2506" },
      { 0x01e2, { 0xc5, 0x16, 0x67, 0x00 },       "lds    dx,DWORD PTR ds:0x67" },
      { 0x01e6, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x01e8, { 0x1f },                         "pop    ds" },
      { 0x01e9, { 0xcb },                         "retf" },
      { 0x01ea, { 0x81, 0xfe, 0x2e, 0x45 },       "cmp    si,0x452e" },
      { 0x01ee, { 0x74, 0x04 },                   "je     0x1f4" },
      { 0x01f0, { 0x32, 0xe4 },                   "xor    ah,ah" },
      { 0x01f2, { 0xeb, 0x02 },                   "jmp    0x1f6" },
      { 0x01f4, { 0xb4, 0xff },                   "mov    ah,0xff" },
      { 0x01f6, { 0x8b, 0xd7 },                   "mov    dx,di" },
      { 0x01f8, { 0x8b, 0xde },                   "mov    bx,si" },
      { 0x01fa, { 0x3b, 0xdf },                   "cmp    bx,di" },
      { 0x01fc, { 0x74, 0x23 },                   "je     0x221" },
      { 0x01fe, { 0x26, 0x80, 0x3f, 0xff },       "cmp    BYTE PTR es:[bx],0xff" },
      { 0x0202, { 0x74, 0x18 },                   "je     0x21c" },
      { 0x0204, { 0x81, 0xfe, 0x2e, 0x45 },       "cmp    si,0x452e" },
      { 0x0208, { 0x74, 0x06 },                   "je     0x210" },
      { 0x020a, { 0x26, 0x3a, 0x67, 0x01 },       "cmp    ah,BYTE PTR es:[bx+0x1]" },
      { 0x020e, { 0xeb, 0x04 },                   "jmp    0x214" },
      { 0x0210, { 0x26, 0x38, 0x67, 0x01 },       "cmp    BYTE PTR es:[bx+0x1],ah" },
      { 0x0214, { 0x77, 0x06 },                   "ja     0x21c" },
      { 0x0216, { 0x26, 0x8a, 0x67, 0x01 },       "mov    ah,BYTE PTR es:[bx+0x1]" },
      { 0x021a, { 0x8b, 0xd3 },                   "mov    dx,bx" },
      { 0x021c, { 0x83, 0xc3, 0x06 },             "add    bx,0x6" },
      { 0x021f, { 0xeb, 0xd9 },                   "jmp    0x1fa" },
      { 0x0221, { 0x3b, 0xd7 },                   "cmp    dx,di" },
      { 0x0223, { 0x74, 0x1b },                   "je     0x240" },
      { 0x0225, { 0x8b, 0xda },                   "mov    bx,dx" },
      { 0x0227, { 0x26, 0x80, 0x3f, 0x00 },       "cmp    BYTE PTR es:[bx],0x0" },
      { 0x022b, { 0x26, 0xc6, 0x07, 0xff },       "mov    BYTE PTR es:[bx],0xff" },
      { 0x022f, { 0x06 },                         "push   es" },
      { 0x0230, { 0x74, 0x07 },                   "je     0x239" },
      { 0x0232, { 0x26, 0xff, 0x5f, 0x02 },       "callf  DWORD PTR es:[bx+0x2]" },
      { 0x0236, { 0x07 },                         "pop    es" },
      { 0x0237, { 0xeb, 0xb1 },                   "jmp    0x1ea" },
      { 0x0239, { 0x26, 0xff, 0x57, 0x02 },       "call   WORD PTR es:[bx+0x2]" },
      { 0x023d, { 0x07 },                         "pop    es" },
      { 0x023e, { 0xeb, 0xaa },                   "jmp    0x1ea" },
      { 0x0240, { 0xc3 },                         "ret" },
      { 0x0241, { 0xb4, 0x40 },                   "mov    ah,0x40" },
      { 0x0243, { 0xbb, 0x02, 0x00 },             "mov    bx,0x2" },
      { 0x0246, { 0xcd, 0x21 },                   "int    0x21" },
      { 0x0248, { 0xc3 },                         "ret" },
      { 0x0249, { 0xb9, 0x1e, 0x00 },             "mov    cx,0x1e" },
      { 0x024c, { 0xba, 0x3d, 0x00 },             "mov    dx,0x3d" },
      { 0x024f, { 0x2e, 0x8e, 0x1e, 0x60, 0x02 }, "mov    ds,WORD PTR cs:0x260" },
      { 0x0254, { 0xe8, 0xea, 0xff },             "call   0x241" },
      { 0x0257, { 0xb8, 0x03, 0x00 },             "mov    ax,0x3" },
      { 0x025a, { 0x50 },                         "push   ax" },
      { 0x025b, { 0x90 },                         "nop" },
      { 0x025c, { 0x0e },                         "push   cs" },
      { 0x025d, { 0xe8, 0xcd, 0x00 },             "call   0x32d" },
      { 0x0034, { 0xf2, 0xae },                   "repne scas   al,BYTE PTR es:[di]" },
      { 0x00b9, { 0xf3, 0xaa },                   "rep stos   BYTE PTR es:[di],al" },
      { 0x0000, { 0x6b, 0xc0, 0x06 },             "imul   ax,ax,0x6" },
      { 0x0000, { 0x6b, 0xff, 0x07 },             "imul   di,di,0x7" },
      { 0x0000, { 0x6b, 0x1e, 0x79, 0x1e, 0x6b }, "imul   bx,WORD PTR ds:0x1e79,0x6b" },
      { 0x0000, { 0x69, 0xf6, 0xa0, 0x00 },       "imul   si,si,0xa0" },
      { 0x0000, { 0x69, 0x01, 0x79, 0x01 },       "imul   ax,WORD PTR ds:[bx+di],0x179" },
    };

    size_t good = 0;

    for (size_t i = 0; i < tests.size(); ++i)
    {
      const TestCase_t& test = tests.at(i);
      SegOff_t addr { { Seg_t::Normal, 0 }, Off_t(test.addr) };
      RegionIter_t bin { segment<uint8_t>
                       { const_cast<void*>(reinterpret_cast<const void*>(test.dat.data())),
                        test.dat.size() },
                       addr };

      auto val = decode::decode_one(bin);
      if(val.is_err())
      {
        std::cout << "error; \"" << val.error() << "\"" << std::endl;
        std::cout.flush();
      }

      assert(val.is_ok());
      auto& p = *val;
      assert(p.has_value());
      {
        Instruction_t& ins = p->first;
        segment<uint8_t>& bytes = p->second;
        std::string str = intel_syntax::format(addr, ins, bytes, false);

        if(str != test.str)
          std::cout << std::format<"Failed ({}/{}) | Expected: '{}' | Got: '{}'\n\nRAW:\n{}">(i, tests.size(), test.str, str, ins.to_str())
                    << std::endl;
        else
        {
          good++;
          //std::cout << std::format<"Passed ({}/{}) | Decoded: '{}'">(i, tests.size(), test.str) << std::endl;
        }
      }
    }

    std::cout << std::format<"decode test passed/total: ({}/{})">(good, tests.size())
              << std::endl;
  }
}
#endif

