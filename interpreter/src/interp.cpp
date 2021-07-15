#include "interp.hpp"
#include "types.hpp"

#include <cassert>
#include <daisa/instruction.hpp>

using namespace daisa;
using namespace daisa::interpreter;

void interpret(
  Memory& mem,
  u16 startAddr,
  std::function<bool(Memory const&, RegisterPage const&)> pollInterrupt
) {
  bool halt = false;
  bool intEnabled = true;
  RegisterPage registers;

  auto toSegmented = [](u16 addr) {
    struct ret {
      u8 seg;
      u8 off;
    };
    return ret{
      static_cast<u8>((0xff00 & addr) >> 8),
      static_cast<u8>(0xff & addr)
    };
  };

  auto realAddr = [](u8 s, u8 p) { return ((u16)s << 8) | p; };

  auto setIP = [&](u16 addr) {
    auto [cs, ip] = toSegmented(addr);
    registers.cs = cs;
    registers.ip = ip;
  };
  setIP(startAddr);

  while (!halt) {
    auto addr = realAddr(registers.cs, registers.ip);
    auto disasm = Instruction::disassemble(std::span{&mem.direct[addr], static_cast<std::size_t>((256*256)-addr)});
    setIP(disasm.continueFrom.data() - &mem.direct[0]);
    if (!disasm) {
      switch (disasm.reason) {
        case FailureReason::InvalidOpCode:
        case FailureReason::NoImmediate:
        case FailureReason::InvalidArgument:
        case FailureReason::NoData:
          // TODO: do something more fun on disassembly failure
          halt = true;
          continue;
        case FailureReason::None:
          // this should never be reached
          break;
      }
    }

    auto insn = *disasm.instruction;
    auto getRegArg = [&] {
      if (auto reg = insn.reg_argument(); reg == Register::Imm) {
        return insn.immedidate();
      } else {
        return registers.addressable[static_cast<u8>(reg)];
      }
    };

    auto pushStack = [&](u8 val) {
      mem.paged[registers.ss][registers.named.sp] = val;
      if (registers.named.sp++ == 0xff)
        registers.ss++;
    };
    auto popStack = [&]() {
      if (registers.named.sp-- == 0xff)
        registers.ss--;
      return mem.paged[registers.ss][registers.named.sp];
    };

    switch (insn.opcode()) {
      case OpCode::NOP:
        break; // do nothing
      case OpCode::JF:
        // cs <- a, ip <- r
        registers.cs = registers.a;
        registers.ip = getRegArg();
        break;
      case OpCode::JN:
        // ip <- r
        registers.ip = getRegArg();
        break;
      case OpCode::Jc:
        { // conditional near jump to immediate
          auto cond = insn.cond_argument();
          bool negated = (static_cast<u8>(cond) & 0b1) != 0;
          auto flag = static_cast<Condition>(static_cast<u8>(cond) & 0b110);
          bool value = [&] {
            switch (flag) {
              case Condition::Zero: return registers.flags.z;
              case Condition::Carry: return registers.flags.c;
              case Condition::Overflow: return registers.flags.o;
              case Condition::Negative: return registers.flags.n;
              default: assert(false); __builtin_unreachable();
            }
          }();
          if (value ^ negated)
            registers.ip = insn.immedidate();
        }
        break;
      // TODO: rest of these
      case OpCode::ENI:
        intEnabled = true;
        break;
      case OpCode::DSI:
        intEnabled = false;
        break;
      case OpCode::IRET:
        registers.ip = popStack();
        registers.cs = popStack();
        intEnabled = true;
        break;
      case OpCode::HLT:
        halt = true;
        continue;
    }

    // check for an interrupt after each instruction
    if (intEnabled && pollInterrupt(mem, registers)) {
      auto seg = mem.paged[0xff][0xfe];
      auto off = mem.paged[0xff][0xff];
      intEnabled = false;
      pushStack(registers.cs);
      pushStack(registers.ip);
      registers.cs = seg;
      registers.ip = off;
    }
  }
}
