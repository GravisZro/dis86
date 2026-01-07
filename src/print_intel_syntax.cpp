#include "dis86.h"

#include <string>
#include <format>
static void print_operand_intel_syntax(std::string& s, dis86_instr_t *ins, const operand_t& o)
{
  switch (o.type) {
    case OPERAND_TYPE_REG: s += reg_name(o.u.reg.id); break;
    case OPERAND_TYPE_MEM: {
      const operand_mem_t& m = o.u.mem;
      switch (m.sz) {
        case SIZE_8:  s += "BYTE PTR "; break;
        case SIZE_16: s += "WORD PTR "; break;
        case SIZE_32: s += "DWORD PTR "; break;
      }
      s += reg_name(m.sreg);
      s += ":";
      if (!m.reg1 && !m.reg2) {
        if (m.off)
          s += m.off;
      } else {
        s += "[";
        if (m.reg1)
          s += reg_name(m.reg1);

        if (m.reg2)
        {
          s += "+";
          s += reg_name(m.reg2);
        }
        if (m.off) {
          int16_t disp = (int16_t)m.off;
          if (disp >= 0)
            s += std::format<"+0x%x">((uint16_t)disp);
          else
            s += std::format<"-0x%x">((uint16_t)-disp);
        }
        s += "]";
      }
    } break;
    case OPERAND_TYPE_IMM:
      s += std::format<"0x%x">(o.u.imm.val);
      break;

    case OPERAND_TYPE_REL:
    {
      uint16_t effective = ins->addr + ins->n_bytes + o.u.rel.val;
      s += std::format<"0x%x">(effective);
    } break;
    case OPERAND_TYPE_FAR:
      s += std::format<"0x%x:0x%x">(o.u.far.seg, o.u.far.off);
      break;
    default:
      FAIL("INVALID OPERAND TYPE: %d", o.type);
  }
}

std::string dis86_print_intel_syntax(dis86_t *d, dis86_instr_t *ins, bool with_detail)
{
  std::string s;
  if (with_detail) {
    s += std::format<"%8zx:\t">(ins->addr);
    for (size_t i = 0; i < ins->n_bytes; i++)
    {
      s += std::format<"%02x ">(binary_byte_at(d->b, ins->addr + i));
    }
    size_t used = ins->n_bytes * 3;
    size_t remain = (used <= 21) ? 21 - used : 0;

    s += std::format<"%*s\t">((int)remain, " ");
  }

  if (ins->rep == REP_NE)
    s += "repne ";
  else if (ins->rep == REP_E)
    s += "rep ";

  s += std::format<"%-5s">(instr_op_mneumonic[int(ins->opcode)]);

  int n_operands = 0;
  for (size_t i = 0; i < ins->operand.size(); i++)
  {
    const operand_t& o = ins->operand[i];
    if (o.type == OPERAND_TYPE_NONE)
      break;
    if ((int)(1<<i) & ins->intel_hidden)
      continue;

    if (n_operands == 0)
      s += "  ";
    else
      s += ",";
    print_operand_intel_syntax(s, ins, o);
    n_operands++;
  }

  /* remove any trailing space */  
  if(size_t pos = s.rfind(' ');
      pos != std::string::npos)
    s.erase(pos);
  return s;
}

char *dis86_print_c_code(dis86_t *d, dis86_instr_t *ins, size_t addr, size_t n_bytes)
{
  UNIMPL();
}
