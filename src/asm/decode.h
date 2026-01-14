#pragma once
#include <cstdint>

#include "common/result.h"
#include "instr.h"
#include "region.h"


namespace decode
{
  using namespace instr;
  using namespace region;
  // Mode is the 2-bits from [6..7] in the ModRM byte
  constexpr uint8_t modrm_mode(uint8_t modrm) { return modrm >> 6; }

  // Reg is the 3-bits from [3..5] in the ModRM byte
  constexpr uint8_t modrm_reg(uint8_t modrm) { return (modrm >> 3) & 7; }

  // Opcode2 is the same as the reg-field in the ModRM byte
  // That is: the 3-bits from [3..5] in the ModRM byte
  constexpr uint8_t modrm_op2(uint8_t modrm) { return (modrm >> 3) & 7; }

  // RM is the 3-bits from [0..2] in the ModRM byte
  constexpr uint8_t modrm_rm(uint8_t modrm) { return modrm & 7; }

  Result<Operand_t, std::string> operand_reg  (Register_t r);
  Result<Operand_t, std::string> operand_imm8 (uint8_t imm);
  Result<Operand_t, std::string> operand_imm16(uint16_t imm);
  Result<Operand_t, std::string> operand_imm16(uint16_t imm, Size_e sz);
  Result<Operand_t, std::string> operand_rel  (RegionIter_t& bin, Size_e sz);
  Result<Operand_t, std::string> operand_src  (Size_e sz);
  Result<Operand_t, std::string> operand_dst  (Size_e sz);
  Result<Operand_t, std::string> operand_far  (RegionIter_t& bin);
  Result<Operand_t, std::string> operand_moff (RegionIter_t& bin, Size_e sz, std::optional<Register_t> prefix_sreg);
  Result<Operand_t, std::string> operand_rm   (RegionIter_t& bin, Size_e sz,  uint8_t modrm, std::optional<Register_t> prefix_sreg);
  Result<Operand_t, std::string> operand_rm8  (RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg);
  Result<Operand_t, std::string> operand_rm16 (RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg);
  Result<Operand_t, std::string> operand_m8   (RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg);
  Result<Operand_t, std::string> operand_m16  (RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg);
  Result<Operand_t, std::string> operand_m32  (RegionIter_t& bin, uint8_t modrm, std::optional<Register_t> sreg);

  Result<std::optional<std::pair<Instruction_t, segment<uint8_t>>>, std::string> decode_one(RegionIter_t& bin);

  struct Decoder
  {
    RegionIter_t bin;
    Result<std::optional<std::pair<Instruction_t, segment<uint8_t>>>, std::string> last;

    Decoder(RegionIter_t& o) : bin (o), last(std::string{}) { }

    Result<std::optional<std::pair<Instruction_t, segment<uint8_t>>>, std::string> try_next(void)
      { return decode_one(bin); }

      bool at_end(void)
      {
        last = try_next();
        return last.is_err() || !*last;
      }
  };

#if 0

impl<a> Iterator for Decoder<a> {
  type Item = (Instr, &a [u8]);
  fn next(&mut self) -> Option<(Instr, &a [u8])> {
    self.try_next().unwrap()
  }
}
#endif

}
