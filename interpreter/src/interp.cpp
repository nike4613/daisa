#include "interp.hpp"
#include "types.hpp"

#include <cassert>
#include <daisa/instruction.hpp>

using namespace daisa;
using namespace daisa::interpreter;

void daisa::interpreter::interpret(
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
    auto regArg = [&]() -> decltype(auto) {
      return registers.addressable[static_cast<u8>(insn.reg_argument())];
    };
    auto getArg = [&]() {
      if (auto reg = insn.reg_argument(); reg == Register::Imm) {
        return insn.immedidate();
      } else {
        return regArg();
      }
    };

    auto regAddr = [&]() -> decltype(auto) {
      auto off = getArg();
      auto seg = [&] {
        if (auto reg = insn.reg_argument(); reg == Register::SP || reg == Register::BP) {
          return registers.ss;
        } else {
          return registers.ds;
        }
      }();
      return mem.paged[seg][off];
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

    auto updateFlags = [&](u8 val) {
      registers.flags.z = val == 0;
      registers.flags.n = (val & 0x80) != 0;
    };
    auto updateFlags2 = [&](u8 val, u8 old, u8 arg) {
      updateFlags(val);
      registers.flags.o = 
        (old & 0x80) == (arg & 0x80)
        && (old & 0x80) != (val & 0x80);
    };
    
    auto addVal = [&](u8& val, u8 amt, bool carry) {
      auto sum = static_cast<i16>(val) + static_cast<i16>(amt) + (carry ? 1 : 0);
      updateFlags2(sum, val, amt);
      registers.flags.c = (sum & 0x100) != 0;
      val = static_cast<u8>(sum & 0xff);
    };
    auto subVal = [&](u8& val, u8 amt) {
      auto sum = static_cast<i16>(val) - static_cast<i16>(amt);
      updateFlags2(sum, val, amt);
      registers.flags.c = (sum & 0x100) != 0;
      val = static_cast<u8>(sum & 0xff);
    };

    bool queueIntEnable = false;
    switch (insn.opcode()) {
      case OpCode::NOP:
        break; // do nothing
      case OpCode::JF:
        // cs <- a, ip <- r
        registers.cs = registers.a;
        registers.ip = getArg();
        break;
      case OpCode::JN:
        // ip <- r
        registers.ip = getArg();
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

      case OpCode::CALLN:
        {
          registers.csr = registers.cs;
          auto tmp = registers.ip;
          registers.ip = getArg();
          registers.named.lr = tmp;
        }
        break;
      case OpCode::CALLF:
        {
          registers.csr = registers.cs;
          registers.cs = registers.a;
          auto tmp = registers.ip;
          registers.ip = getArg();
          registers.named.lr = tmp;
        }
        break;
      case OpCode::RET:
        registers.cs = registers.csr;
        registers.ip = registers.named.lr;
        break;

      case OpCode::PUSH:
        pushStack(getArg());
        break;
      case OpCode::PUSH_CSR:
        pushStack(registers.csr);
        break;
      case OpCode::POP:
        regArg() = popStack();
        break;
      case OpCode::POP_CSR:
        registers.csr = popStack();
        break;

      case OpCode::LDA_CSR:
        registers.a = registers.csr;
        break;
      case OpCode::STA_CSR:
        registers.csr = registers.a;
        break;

      case OpCode::LDDS:
        registers.ds = getArg();
        break;
      case OpCode::STDS:
        regArg() = registers.ds;
        break;
      case OpCode::LDSS:
        registers.ss = getArg();
        break;
      case OpCode::STSS:
        regArg() = registers.ss;
        break;

      case OpCode::LDA:
        registers.a = getArg();
        break;
      case OpCode::STA:
        regArg() = registers.a;
        break;
      case OpCode::LDM:
        registers.a = regAddr();
        break;
      case OpCode::STM:
        regAddr() = registers.a;
        break;

      case OpCode::SWP:
        std::swap(registers.a, regArg());
        break;
      case OpCode::INC_A:
        addVal(registers.a, 1, false);
        break;
      case OpCode::DEC_A:
        subVal(registers.a, 1);
        break;
      case OpCode::INC:
        addVal(regArg(), 1, false);
        break;
      case OpCode::DEC:
        subVal(regArg(), 1);
        break;
      case OpCode::ADC:
        addVal(registers.a, getArg(), registers.flags.c);
        break;
      case OpCode::ADD:
        addVal(registers.a, getArg(), false);
        break;
      case OpCode::SUB:
        subVal(registers.a, getArg());
        break;
      case OpCode::SHL:
        registers.flags.c = (registers.a & 0x80) != 0;
        updateFlags(registers.a <<= 1);
        break;
      case OpCode::SHR:
        registers.flags.c = false;
        updateFlags(registers.a >>= 1);
        break;
      case OpCode::SRA:
        {
          registers.flags.c = false;
          auto v = static_cast<i8>(registers.a);
          v >>= 1;
          updateFlags(registers.a = static_cast<u8>(v));
        }
        break;
      case OpCode::ROL:
        {
          auto& a = registers.a;
          a = (a << 1) | ((a & 0x80) >> 7);
          updateFlags(a);
        }
        break;
      case OpCode::ROR:
        {
          auto& a = registers.a;
          a = (a >> 1) | ((a & 0x01) << 7);
          updateFlags(a);
        }
        break;
      case OpCode::AND:
        registers.flags.c = registers.flags.o = false;
        updateFlags(registers.a &= getArg());
        break;
      case OpCode::OR:
        registers.flags.c = registers.flags.o = false;
        updateFlags(registers.a |= getArg());
        break;
      case OpCode::XOR:
        registers.flags.c = true;
        registers.flags.o = false;
        updateFlags(registers.a ^= getArg());
        break;
      case OpCode::CLR:
        updateFlags(registers.a = 0);
        break;
      case OpCode::CFLAGS:
        registers.flags = {};
        break;

      case OpCode::ENI:
        queueIntEnable = true;
        break;
      case OpCode::DSI:
        intEnabled = false;
        break;
      case OpCode::IRET:
        registers.ip = popStack();
        registers.cs = popStack();
        queueIntEnable = true;
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

    if (queueIntEnable)
      intEnabled = true;
  }
}
