#include "opcode.h"

#include <array>
#include <cassert>

namespace opcode
{
  std::string_view to_string(Opcode value)
  {
    const std::array<std::string_view, 76> strs =
    {
      {
         "nop",         "pin",         "ref",         "phi",      "unimpl",
         "sub",         "add",         "shl",
         "shr",        "ushr",
         "and",          "or",         "xor",
        "imul",        "umul",
        "idiv",        "udiv",
         "neg",
         "not",
   "signext32",

          // register manipulation
       "load8",      "load16",      "load32",
      "store8",     "store16",     "store32",
    "readvar8",   "readvar16",   "readvar32",
   "writevar8",  "writevar16",  "writevar32",
    "readarr8",   "readarr16",   "readarr32",
   "writearr8",  "writearr16",

     "lower16",
     "upper16",
      "make32",

          // flags
      "updf", // update
       "eqf", // ==
      "neqf", // !=
       "gtf", // >
      "geqf", // >=
       "ltf", // <
      "leqf", // <=
      "ugtf", // (unsigned) >
     "ugeqf", // (unsigned) >=
      "ultf", // (unsigned) <
     "uleqf", // (unsigned) <=
     "signf", // is signed flag

          // operations
        "eq",     "neq", // (any sign) ==, (any sign) !=
        "gt",     "geq", // (signed) >, (signed)>=
        "lt",     "leq", // (signed) <, (signed) <=
       "ugt",    "ugeq", // (unsigned) >, (unsigned) >=
       "ult",    "uleq", // (unsigned) <, (unsigned) <=
      "sign", "notsign", // is signed, is not signed

          // subroutine
   "callfar",
  "callnear",
   "callptr",
  "callargs",
       "int", // invoke interupt
      "retf", // return far
      "retn", // return near

          // jump
       "jmp",
       "jne",
    "jmptbl",

          // TODO: HMMM.... Better Impl?
   "assert_even",
    "assert_pos",
      }
    };
    assert(static_cast<uint8_t>(value) < strs.size());
    return strs.at(static_cast<uint8_t>(value));
  }
}
