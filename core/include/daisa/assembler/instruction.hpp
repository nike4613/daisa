#pragma once

#include <optional>
#include <vector>
#include <string>
#include <variant>

#include <daisa/types.hpp>
#include <daisa/instruction.hpp>

namespace daisa::assembler {

  using BaseInstruction = daisa::Instruction;

  struct Label {
    std::string name;
    std::optional<u16> boundTo;
  };

  struct LabelRef {
    Label* label;
    enum { Low, High } kind;
  };

  // this is a higher-level type, which also works with labels
  class Instruction {
  private:
    OpCode opcode_;
    std::variant<std::monostate, Register, Condition> argument;
    bool isValid;
    ArgKind needsKind;
    std::variant<std::monostate, u8, LabelRef> immediate;

  public:
    // intentionally implicit; no argument
    constexpr Instruction(OpCode opcode) noexcept
      : opcode_(opcode), isValid(opcode_is_valid(opcode) && !opcode_has_arg(opcode))
    {}
    // typical register argument (doesn't accept imm as a register kind)
    constexpr Instruction(OpCode opcode, Register reg) noexcept
      : opcode_(opcode), argument(reg)
    {
      isValid = opcode_is_valid(opcode)
        && opcode_has_arg(opcode, &needsKind) 
        && needsKind != ArgKind::Cond
        && reg != Register::Imm;
      needsKind = ArgKind::RegOnly; // this is for tracking opcode changes
    }
    // numeric immediate argument
    constexpr Instruction(OpCode opcode, u8 immediate) noexcept
      : opcode_(opcode), argument(Register::Imm), immediate(immediate)
    {
      isValid = opcode_is_valid(opcode)
        && opcode_has_arg(opcode, &needsKind)
        && needsKind == ArgKind::ImmReg;
    }
    // label immediate argument
    constexpr Instruction(OpCode opcode, LabelRef label) noexcept
      : opcode_(opcode), argument(Register::Imm), immediate(label)
    {
      isValid = opcode_is_valid(opcode)
        && opcode_has_arg(opcode, &needsKind)
        && needsKind == ArgKind::ImmReg;
    }
    // conditional jump (cond+labelref)
    constexpr Instruction(OpCode opcode, Condition cond, LabelRef label) noexcept
      : opcode_(opcode), argument(cond), immediate(label)
    {
      isValid = opcode_is_valid(opcode)
        && opcode_has_arg(opcode, &needsKind)
        && needsKind == ArgKind::Cond;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept { return isValid; }
    [[nodiscard]] constexpr OpCode opcode() const noexcept { return opcode_; }

    [[nodiscard]] constexpr bool has_argument() const noexcept { return argument.index() != 0; }
    [[nodiscard]] constexpr bool has_register() const noexcept { return argument.index() == 1; }
    [[nodiscard]] constexpr auto register_arg() const noexcept { return std::get<Register>(argument); }
    [[nodiscard]] constexpr bool has_condition() const noexcept { return argument.index() == 2; }
    [[nodiscard]] constexpr auto condition_arg() const noexcept { return std::get<Condition>(argument); }

    [[nodiscard]] constexpr bool has_immediate() const noexcept { return immediate.index() != 0; }
    [[nodiscard]] constexpr bool has_literal() const noexcept { return immediate.index() == 1; }
    [[nodiscard]] constexpr auto literal() const noexcept { return std::get<u8>(immediate); }
    [[nodiscard]] constexpr bool has_label() const noexcept { return immediate.index() == 2; }
    [[nodiscard]] constexpr auto label() const noexcept { return std::get<LabelRef>(immediate); }

    [[nodiscard]] constexpr Instruction with_opcode(OpCode opcode) const noexcept;
    [[nodiscard]] constexpr Instruction with_register(Register reg) const noexcept;
    [[nodiscard]] constexpr Instruction with_condition(Condition cond) const noexcept;
    [[nodiscard]] constexpr Instruction with_literal(u8 literal) const noexcept;
    [[nodiscard]] constexpr Instruction with_label(LabelRef label) const noexcept;
    
    [[nodiscard]] constexpr Instruction with_argument(Register reg) const noexcept;
    [[nodiscard]] constexpr Instruction with_argument(u8 literal) const noexcept;
    [[nodiscard]] constexpr Instruction with_argument(LabelRef label) const noexcept;
    [[nodiscard]] constexpr Instruction with_argument(Condition cond, LabelRef label) const noexcept;

  };

  // implementation

  constexpr Instruction Instruction::with_opcode(OpCode opcode) const noexcept {
    if (has_argument()) {
      if (has_condition()) { // implies has_immediate() and has_label()
        return Instruction(opcode, condition_arg(), label());
      } else if (has_immediate()) {
        if (has_literal()) {
          return Instruction(opcode, literal());
        } else { // has_label()
          return Instruction(opcode, label());
        }
      } else { // has_register() && !has_immediate()
        return Instruction(opcode, register_arg());
      }
    } else {
      return Instruction(opcode);
    }
  }

  constexpr Instruction Instruction::with_register(Register reg) const noexcept {
    return Instruction(opcode(), reg);
  }
  constexpr Instruction Instruction::with_condition(Condition cond) const noexcept {
    return Instruction(opcode(), cond, label());
  }
  constexpr Instruction Instruction::with_literal(u8 lit) const noexcept {
    return Instruction(opcode(), lit);
  }
  constexpr Instruction Instruction::with_label(LabelRef label) const noexcept {
    if (has_condition()) {
      return Instruction(opcode(), condition_arg(), label);
    } else {
      return Instruction(opcode(), label);
    }
  }

  constexpr Instruction Instruction::with_argument(Register reg) const noexcept {
    return Instruction(opcode(), reg);
  }
  constexpr Instruction Instruction::with_argument(u8 lit) const noexcept {
    return Instruction(opcode(), lit);
  }
  constexpr Instruction Instruction::with_argument(LabelRef label) const noexcept {
    return Instruction(opcode(), label);
  }
  constexpr Instruction Instruction::with_argument(Condition cond, LabelRef label) const noexcept {
    return Instruction(opcode(), cond, label);
  }

}