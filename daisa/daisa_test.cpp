#include <daisa.hpp>
#include <iostream>

#include <string>
#include <array>

bool instruction_test() {
    using namespace daisa;

    std::array<u8, 2> data{ 0b11000000, 0b11000001 };
    auto result = Instruction::disassemble(data);
    if (!result) return false;
    if (!result.instruction.has_value()) return false;
    if (result.instruction->opcode() != OpCode::NOP) return false;
    auto result2 = Instruction::disassemble(result.continueFrom);
    if (!result2) return false;
    if (!result2.instruction.has_value()) return false;
    if (result2.instruction->opcode() != OpCode::RET) return false;

    return 
        opcode_has_arg(OpCode::LDA_CSR) == std::nullopt &&
        opcode_has_arg(OpCode::POP).value_or(static_cast<ArgKind>(-1)) == ArgKind::RegOnly &&
        true;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << argv[0] << " takes one argument.\n";
        return 1;
    }
    if (std::string(argv[1]) == "instruction")
        return !instruction_test();

    std::cout << "Unrecognized test." << std::endl;
    return 0;
}