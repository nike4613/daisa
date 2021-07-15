
#include "types.hpp"
#include "interp.hpp"

#include <memory>
#include <utility>

int main(int argc, char const* const* argv) {
  auto mem = std::make_unique<daisa::interpreter::Memory>();
  mem->direct = {
    0b11001111, // dsi ; make sure the interrupt isn't triggered while we set up the routine
    0b10000000, 0xff, // ldds 0xff ; set the data segment
    0b01000000, 0xff, // lda 0xff ; load our interrupt routine data into place
    0b01011000, 0xfe, // stm 0xfe
    0b01000000, 0xfd, // lda 0xfd
    0b01011000, 0xff, // stm 0xff
    0b01000000, 0b11010000, // lda (iret) ; load a simple (as in, only iret) handler
    0b01011000, 0xfd, // stm 0xfd
    0b10010000, 0x80, // ldss 0x08 ; load our stack segment
    0b11001000, // clr
    0b01001110, // sta sp ; zero stack pointer
    0b11001110, // eni ; we're done with our core setup

    // ; do a bunch of nops so we can see the interrupt, then halt
    0b11000000, // nop
    0b11000000, // nop
    0x00, 0x00, // xor 0
    0x00, 0x00, // xor 0
    0b11000000, // nop
    0x00, 0x00, // xor 0
    0b11000000, // nop
    0x00, 0x00, // xor 0
    0b11000000, // nop
    0b11000000, // nop
    0x00, 0x00, // xor 0
    0x00, 0x00, // xor 0
    0b11000000, // nop
    0x00, 0x00, // xor 0
    0b11000000, // nop
    0x00, 0x00, // xor 0

    // ; halt
    0b11001011
  };

  daisa::interpreter::interpret(*mem, 0, [](auto const& mem, auto const& regs) {
    return true; // always interrupt
  });

  return 0;
}

