#pragma once

#include <optional>
#include <span>
#include <array>
#include <cassert>

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
    #define INSN_NARG(name, bits) name = detail::noarg_op(bits),
    #define INSN_ARG_(name, bits, kind) name = detail::arg_op(bits, kind),
    #define KIND_IMMREG ArgKind::ImmReg
    #define KIND_REG ArgKind::RegOnly
    #define KIND_COND ArgKind::Cond
    #include <daisa/isa.inc>
    #undef KIND_IMMREG
    #undef KIND_REG
    #undef KIND_COND
    #undef INSN_NARG
    #undef INSN_ARG_
  };

  [[nodiscard]] inline constexpr bool opcode_is_valid(OpCode opcode) {
    #define INSN_ANY(name) case OpCode::name: return true;
    switch (opcode) {
      #include <daisa/isa.inc>
    default: return false;
    }
    #undef INSN_ANY
    return false;
  }

  /// @brief Gets whether the opcode has an argumment and may return the kind of argument if it does.
  /// @param[in]  opcode  The opcode to get argument information for.
  /// @param[out] kind    If not null, is set to the kind of argument the opcode takes. If it takes nothing, is unchanged.
  /// @return             Whether or not the opcode takes an argument.
  [[nodiscard]] inline constexpr bool opcode_has_arg(OpCode opcode, ArgKind* kind) noexcept {
    using namespace detail;
    if ((static_cast<u8>(opcode) & noarg_check_bits) == noarg_check_bits) {
      // no argument
      return false;
    }
    // our argument is the low 3 bits
    if (kind) *kind = static_cast<ArgKind>(static_cast<u8>(opcode) & 0b111);
    return true;
  }

  /// @brief Gets whether the opcode has an argumment.
  /// @param[in]  opcode  The opcode to get argument information for.
  /// @return             Whether or not the opcode takes an argument.
  [[nodiscard]] inline constexpr bool opcode_has_arg(OpCode opcode) noexcept {
    return opcode_has_arg(opcode, nullptr);
  }

  /// @brief Gets the kind of argument an opcode has.
  /// @note The behaviour of this function is undefined if the opcode doesn't actually take an argument.
  /// @param[in]  opcode  The opcode to get the argument kind of.
  /// @return             The ArgKind that the argument takes.
  [[nodiscard]] inline constexpr ArgKind opcode_get_arg(OpCode opcode) noexcept {
    ArgKind kind;
    auto opcodeHasArg = opcode_has_arg(opcode, &kind);
    assert(opcodeHasArg);
    return kind;
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
    Zero = 0b000,
    NotZero = 0b001,

    Carry = 0b010,
    NotCarry = 0b011,

    Overflow = 0b100,
    NotOverflow = 0b101,

    E = 0b110,
    NotE = 0b111,
  };

  struct DisassemblyResult;

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

    [[nodiscard]] constexpr u8 length() const noexcept { return hasImm ? 2 : 1; }

    [[nodiscard]] constexpr OpCode opcode() const noexcept { return opcode_; }
    [[nodiscard]] constexpr bool has_immediate() const noexcept { return hasImm; }
    [[nodiscard]] constexpr u8 immedidate() const noexcept { return immediateValue; }
    [[nodiscard]] constexpr bool has_argument() const noexcept { return argInfo != argInfo_none; }
    [[nodiscard]] constexpr bool has_reg_argument() const noexcept { return argInfo == argInfo_reg; }
    [[nodiscard]] constexpr bool has_cond_argument() const noexcept { return argInfo == argInfo_cond; }
    [[nodiscard]] constexpr Register reg_argument() const noexcept { return arg.reg; }
    [[nodiscard]] constexpr Condition cond_argument() const noexcept { return arg.cond; }

    [[nodiscard]] constexpr u8 encode() const noexcept;
    [[nodiscard]] static constexpr DisassemblyResult disassemble(std::span<u8 const> const&) noexcept;
  };

  enum class FailureReason {
    None,

    InvalidArgument,
    InvalidOpCode,
    NoData,
    NoImmediate,
  };

  struct DisassemblyResult {
    std::optional<Instruction> instruction;
    std::span<u8 const> continueFrom;
    FailureReason reason;

    constexpr DisassemblyResult(DisassemblyResult const&) noexcept = default;
    constexpr DisassemblyResult(DisassemblyResult&&) noexcept = default;

    // intentionally implicit
    constexpr DisassemblyResult(FailureReason reason) noexcept : reason(reason) {}
    constexpr DisassemblyResult(FailureReason reason, std::span<u8 const> data) noexcept
    : continueFrom(data), reason(reason) {}
    constexpr DisassemblyResult(Instruction insn, std::span<u8 const> data) noexcept
    : instruction(insn), continueFrom(data), reason(FailureReason::None) {}

    constexpr operator bool() const noexcept { return instruction.has_value(); }
  };

  // implmentations

  constexpr std::optional<Instruction> Instruction::create(OpCode op) noexcept {
    if (opcode_has_arg(op))
      return std::nullopt;
    return Instruction(op);
  }

  constexpr std::optional<Instruction> Instruction::create(OpCode op, Register arg) noexcept {
    ArgKind argKind;
    if (!opcode_has_arg(op, &argKind))
      return std::nullopt; // opcode must take argument
    if (argKind != ArgKind::ImmReg && argKind != ArgKind::RegOnly)
      return std::nullopt; // opcode must take register
    if (arg == Register::Imm)
      return std::nullopt; // if passsing an immediate, must actually give it a value
    return Instruction(op, 0, arg);
  }

  constexpr std::optional<Instruction> Instruction::create(OpCode op, u8 imm) noexcept {
    ArgKind argKind;
    if (!opcode_has_arg(op, &argKind))
      return std::nullopt; // opcode must take argument
    if (argKind != ArgKind::ImmReg)
      return std::nullopt; // opcode must take immediate
    return Instruction(op, imm, Register::Imm);
  }

  constexpr std::optional<Instruction> Instruction::create(OpCode op, Condition cond, u8 imm) noexcept {
    ArgKind argKind;
    if (!opcode_has_arg(op, &argKind))
      return std::nullopt; // opcode must take argument
    if (argKind != ArgKind::Cond)
      return std::nullopt; // opcode must take condition
    return Instruction(op, imm, cond);
  }

  constexpr u8 Instruction::encode() const noexcept {
    if (!has_argument())
      return static_cast<u8>(opcode());

    auto argbits = has_reg_argument()
      ? static_cast<u8>(reg_argument())
      : static_cast<u8>(cond_argument());

    return (static_cast<u8>(opcode()) & 0b11111000) | (argbits & 0b111);
  }

  constexpr DisassemblyResult Instruction::disassemble(std::span<u8 const> const& data) noexcept {
    if (data.size() <= 0)
      return FailureReason::NoData; // with no data, there's no reason to return the span
    auto opcodeb = data[0];
    auto cont = data.subspan(1);

    using namespace detail;
    if ((opcodeb & noarg_check_bits) == noarg_check_bits) {
      // takes no argument
      auto opcode = static_cast<OpCode>(opcodeb);
      if (!opcode_is_valid(opcode))
        return DisassemblyResult(FailureReason::InvalidOpCode, cont);
      auto insn = create(opcode);
      if (!insn)
        return DisassemblyResult(FailureReason::InvalidArgument, cont);
      return DisassemblyResult(*insn, cont);
    }

    // otherwise, we take an argument
    auto createReg = [&](OpCode op, u8 arg) {
      auto reg = static_cast<Register>(arg & 0b111);
      if (reg == Register::Imm) {
        // specifically check for this and return early so as to not try to consume the immediate
        if (opcode_get_arg(op) != ArgKind::ImmReg)
          return DisassemblyResult(FailureReason::InvalidArgument, cont);
        if (cont.size() <= 0)
          return DisassemblyResult(FailureReason::NoImmediate, cont);
        auto imm = cont[0];
        cont = cont.subspan(1);
        auto insn = create(op, imm);
        if (!insn)
          return DisassemblyResult(FailureReason::InvalidArgument, cont);
        return DisassemblyResult(*insn, cont);
      }
      auto insn = create(op, reg);
      if (!insn)
        return DisassemblyResult(FailureReason::InvalidArgument, cont);
      return DisassemblyResult(*insn, cont);
    };
    auto createCond = [&](OpCode op, u8 arg) {
      auto cond = static_cast<Condition>(arg & 0b111);
      // the only condition opcode takes an immediate too
      if (cont.size() <= 0)
        return DisassemblyResult(FailureReason::NoImmediate);
      auto imm = cont[0];
      cont = cont.subspan(1);
      auto insn = create(op, cond, imm);
      if (!insn)
        return DisassemblyResult(FailureReason::InvalidArgument, cont);
      return DisassemblyResult(*insn, cont);
    };

    #define KIND_IMMREG createReg
    #define KIND_REG createReg
    #define KIND_COND createCond
    #define INSN_ARG(name, bits, kind) \
      case bits: \
        return kind(OpCode::name, opcodeb & 0b111);
    switch ((opcodeb & 0b11111000) >> 3) {
      #include <daisa/isa.inc>
      default:
        return DisassemblyResult(FailureReason::InvalidOpCode, cont);
    }
    #undef KIND_IMMREG
    #undef KIND_REG
    #undef KIND_CONT
    #undef INSN_ARG
  }


  struct AssembleResult {
    std::array<u8, 256> output;
    std::span<Instruction const> continueWith;
    std::optional<u8> nextFirstByte;

    [[nodiscard]] constexpr bool has_remaining() const noexcept { return continueWith.size() > 0; }
  };

  // implementation

  namespace detail {
    constexpr void assemble_segment(AssembleResult const& lastResult, AssembleResult& result) noexcept {
      auto& outData = result.output;
      auto addr = 0;
      auto write = [&](u8 value) {
        outData[addr++] = value;
      };

      auto insns = lastResult.continueWith;
      auto takeInsn = [&]() {
        auto insn = insns[0];
        insns = insns.subspan(1);
        return insn;
      };

      if (lastResult.nextFirstByte) {
        write(*lastResult.nextFirstByte);
      }
      result.nextFirstByte = {};
      // ^^ the above *must* be in this order, because lastResult and result may be the same

      while (addr < 256 && insns.size() > 0) {
        auto insn = takeInsn();
        write(insn.encode());
        if (insn.has_immediate()) {
          if (addr >= 256) { // we can't write anymore
            result.nextFirstByte = insn.immedidate();
          } else {
            write(insn.immedidate());
          }
        }
      }

      result.continueWith = insns;
    }
  }

  constexpr AssembleResult assemble_segment(std::span<Instruction const> input) noexcept {
    auto result = AssembleResult{ std::array<u8, 256>{}, input, std::nullopt }; // create our value on stack, with NVRO
    detail::assemble_segment(result, result); //   because we weren't given a previous, we want to use the
                                              // same reference to avoid allocating multiple 256-byte blocks
    return result;
  }

  constexpr AssembleResult assemble_segment(AssembleResult const& lastResult) noexcept {
    AssembleResult result; // make sure this NVRO's
    detail::assemble_segment(lastResult, result);
    return result;
  }

}