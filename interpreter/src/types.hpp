#pragma once

#include <array>

#include <daisa/types.hpp>

namespace daisa::interpreter {

  struct RegisterPage {
    u8 a;
    u8 cs;
    u8 ds;
    u8 ss;
    u8 ip;
    u8 csr;

    struct Flags {
      /// zero
      u8 z : 1;
      /// carry
      u8 c : 1;
      /// overflow
      u8 o : 1;
      /// negative
      u8 n : 1;
    };

    struct AddressableRegs {
      Flags flags;
      u8 r1;
      u8 r2;
      u8 r3;
      u8 r4;
      u8 lr;
      u8 sp;
      u8 bp;
    };

    union {
      Flags flags;
      std::array<u8, 8> addressable;
      AddressableRegs named;
    };
  };

  // 256 pages of 256
  union Memory {
    std::array<std::array<u8, 256>, 256> paged;
    std::array<u8, 256*256> direct;
  };
  static_assert(sizeof(Memory) == 256*256, "Unexpected size of Memory object");

}