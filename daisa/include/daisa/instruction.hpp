#pragma once

#include <optional>
#include <daisa/types.hpp>

namespace daisa {

enum class ArgKind : u8 {
  ImmReg = 0b000,
  Cond = 0b001,
  RegOnly = 0b010,
};

namespace detail {
  inline constexpr u8 noarg_check_bits = 0b11000000u;
  inline constexpr u8 noarg_op(u8 val) {
    return noarg_check_bits | (val & ~noarg_check_bits);
  }
  inline constexpr u8 arg_op(u8 val, ArgKind argKind = ArgKind::ImmReg) {
    auto highBits = val & 0b11000;
    if (highBits == 0b11000)
      throw "Arg op cannot have both high bits set";
    return ((val & 0b11111) << 3) 
      | (static_cast<u8>(argKind) & 0b111);
  }
}

enum class OpCode : u8 {
  NOP = detail::noarg_op(0b000000),
  JF = detail::arg_op(0b00001),
  JN = detail::arg_op(0b00010),
  Jc = detail::arg_op(0b00011, ArgKind::Cond),

  CALLN = detail::arg_op(0b00100),
  CALLF = detail::arg_op(0b00101),
  RET = detail::noarg_op(0b000001),
  PUSH = detail::arg_op(0b00110),
  POP = detail::arg_op(0b00111, ArgKind::RegOnly),
  PUSH_CSR = detail::noarg_op(0b001001),
  POP_CSR = detail::noarg_op(0b001010),
  LDA_CSR = detail::noarg_op(0b001100),
  STA_CSR = detail::noarg_op(0b001101),

  LDDS = detail::arg_op(0b10000),
  STDS = detail::arg_op(0b10001, ArgKind::RegOnly),
  LDSS = detail::arg_op(0b10010),
  STSS = detail::arg_op(0b10011, ArgKind::RegOnly),

  LDA = detail::arg_op(0b01000),
  STA = detail::arg_op(0b01001, ArgKind::RegOnly),
  LDM = detail::arg_op(0b01010),
  STM = detail::arg_op(0b01011),

  SWP = detail::arg_op(0b10100, ArgKind::RegOnly),
  INC_A = detail::noarg_op(0b000010),
  DEC_A = detail::noarg_op(0b000011),
  INC = detail::arg_op(0b10101, ArgKind::RegOnly),
  DEC = detail::arg_op(0b10110, ArgKind::RegOnly),
  ADD = detail::arg_op(0b01100),
  SUB = detail::arg_op(0b01101),
  SHL = detail::noarg_op(0b000100),
  SHR = detail::noarg_op(0b000101),
  ROL = detail::noarg_op(0b000110),
  ROR = detail::noarg_op(0b000111),
  AND = detail::arg_op(0b01110),
  OR = detail::arg_op(0b01111),
  XOR = detail::arg_op(0b00000),
  CLR = detail::noarg_op(0b001000),

  ENI = detail::noarg_op(0b001110),
  DSI = detail::noarg_op(0b001111),
  IRET = detail::noarg_op(0b010000),
  
  HLT = detail::noarg_op(0b001011),
};

// @breif
//   Checks whether the opcode has an argumment
// and returns the kind of argument if it does.
[[nodiscard]] inline constexpr std::optional<ArgKind> opcode_has_arg(OpCode opcode) {
  using namespace detail;
  if ((static_cast<u8>(opcode) & noarg_check_bits) == noarg_check_bits) {
    // no argument
    return std::nullopt;
  }
  // our argument is the low 3 bits
  return static_cast<ArgKind>(static_cast<u8>(opcode) & 0b111);
}

enum class Register : u8 {
  Imm = 0b000,
  R1 = 0b001,
  R2 = 0b010,
  R3 = 0b011,
  R4 = 0b100,
  LR = 0b101,
  SP = 0b110,
  BP = 0b111,
};

enum class Condition : u8 {
  // TODO: what conditions need to be available?
};

class Instruction {
private:
  OpCode opcode_;
  u8 immediateValue;
  union {
    Register reg;
    Condition cond;
    u8 none;
  } arg;
  bool hasImm : 1;
  u8 argInfo : 2;

  static constexpr u8 argInfo_none = 0b00;
  static constexpr u8 argInfo_reg = 0b01;
  static constexpr u8 argInfo_cond = 0b10;

  constexpr Instruction(OpCode op, u8 imm, Register r) noexcept
    : opcode_(op), immediateValue(imm), arg{ .reg = r },
      hasImm(r == Register::Imm), argInfo(argInfo_reg)
  {}
  constexpr Instruction(OpCode op, u8 imm, Condition c) noexcept
    : opcode_(op), immediateValue(imm), arg{ .cond = c },
      hasImm(true), argInfo(argInfo_cond)
  {}
  constexpr explicit Instruction(OpCode op) noexcept
    : opcode_(op), immediateValue(0), arg{ .none = 0 },
      hasImm(false), argInfo(argInfo_none)
  {}

public:

  constexpr Instruction(Instruction const&) noexcept = default;
  constexpr Instruction(Instruction&&) noexcept = default;

  // TODO: make these use a result type of some kind
  [[nodiscard]] static constexpr std::optional<Instruction> create(OpCode op) noexcept;
  [[nodiscard]] static constexpr std::optional<Instruction> create(OpCode op, Register arg) noexcept;
  [[nodiscard]] static constexpr std::optional<Instruction> create(OpCode op, u8 arg) noexcept;
  [[nodiscard]] static constexpr std::optional<Instruction> create(OpCode op, Condition cond, u8 arg) noexcept;

  [[nodiscard]] constexpr OpCode opcode() const noexcept { return opcode_; }
  [[nodiscard]] constexpr bool has_immediate() const noexcept { return hasImm; }
  [[nodiscard]] constexpr u8 immedidate() const noexcept { return immediateValue; }
  [[nodiscard]] constexpr bool has_argument() const noexcept { return argInfo != argInfo_none; }
  [[nodiscard]] constexpr bool has_reg_argument() const noexcept { return argInfo == argInfo_reg; }
  [[nodiscard]] constexpr bool has_cond_argument() const noexcept { return argInfo == argInfo_cond; }
  [[nodiscard]] constexpr Register reg_argument() const noexcept { return arg.reg; }
  [[nodiscard]] constexpr Condition cond_argument() const noexcept { return arg.cond; }
};


constexpr std::optional<Instruction> Instruction::create(OpCode op) noexcept {
  if (opcode_has_arg(op))
    return std::nullopt;
  return Instruction(op);
}

constexpr std::optional<Instruction> Instruction::create(OpCode op, Register arg) noexcept {
  auto opArg = opcode_has_arg(op);
  if (!opArg)
    return std::nullopt; // opcode must take argument
  auto argKind = *opArg;
  if (argKind != ArgKind::ImmReg && argKind != ArgKind::RegOnly)
    return std::nullopt; // opcode must take register
  if (arg == Register::Imm)
    return std::nullopt; // if passsing an immediate, must actually give it a value
  return Instruction(op, 0, arg);
}

constexpr std::optional<Instruction> Instruction::create(OpCode op, u8 imm) noexcept {
  auto opArg = opcode_has_arg(op);
  if (!opArg)
    return std::nullopt; // opcode must take argument
  auto argKind = *opArg;
  if (argKind != ArgKind::ImmReg)
    return std::nullopt; // opcode must take immediate
  return Instruction(op, imm, Register::Imm);
}


constexpr std::optional<Instruction> Instruction::create(OpCode op, Condition cond, u8 imm) noexcept {
  auto opArg = opcode_has_arg(op);
  if (!opArg)
    return std::nullopt; // opcode must take argument
  auto argKind = *opArg;
  if (argKind != ArgKind::Cond)
    return std::nullopt; // opcode must take condition
  return Instruction(op, imm, cond);
}

}