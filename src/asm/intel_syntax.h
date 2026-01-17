#pragma once
#include <string>
#include <cstdint>

#include "common/segment.h"
#include "instr.h"
#include "segoff.h"


namespace bin::intel_syntax
{
  using namespace bin::instr;
  using namespace segoff;

  void format_operand(std::string& s, Instruction_t& ins, Operand_t oper);
  std::string format(SegOff_t addr, std::optional<Instruction_t> ins, segment<uint8_t> bytes, bool with_detail);
}
