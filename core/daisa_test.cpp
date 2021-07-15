#include <daisa.hpp>
#include <iostream>

#include <string>
#include <array>
#include <vector>

bool instruction_test() {
    using namespace daisa;

    std::array data{ 
        (u8)0b11000000, // NOP
        (u8)0b11000001, // RET
        (u8)0b00001001, // JF r1
        (u8)0b00001010, // JF r2
        (u8)0b00010000, (u8)0xab, // JN 0xab
        (u8)0b00111001, // POP r1
        (u8)0b00111000, (u8)0xff, // POP 0xff (invalid)
    };
    auto result = Instruction::disassemble(data);
    if (!result) return false;
    if (!result.instruction.has_value()) return false;
    if (result.instruction->opcode() != OpCode::NOP) return false;
    if (result.instruction->encode() != data[0]) return false;
    auto result2 = Instruction::disassemble(result.continueFrom);
    if (!result2) return false;
    if (!result2.instruction.has_value()) return false;
    if (result2.instruction->opcode() != OpCode::RET) return false;
    if (result2.instruction->encode() != data[1]) return false;
    auto result3 = Instruction::disassemble(result2.continueFrom);
    if (!result3) return false;
    if (!result3.instruction.has_value()) return false;
    if (result3.instruction->opcode() != OpCode::JF) return false;
    if (!result3.instruction->has_argument()) return false;
    if (!result3.instruction->has_reg_argument()) return false;
    if (result3.instruction->reg_argument() != Register::R1) return false;
    if (result3.instruction->encode() != data[2]) return false;
    auto result4 = Instruction::disassemble(result3.continueFrom);
    if (!result4) return false;
    if (!result4.instruction.has_value()) return false;
    if (result4.instruction->opcode() != OpCode::JF) return false;
    if (!result4.instruction->has_argument()) return false;
    if (!result4.instruction->has_reg_argument()) return false;
    if (result4.instruction->reg_argument() != Register::R2) return false;
    if (result4.instruction->encode() != data[3]) return false;
    auto result5 = Instruction::disassemble(result4.continueFrom);
    if (!result5) return false;
    if (!result5.instruction.has_value()) return false;
    if (result5.instruction->opcode() != OpCode::JN) return false;
    if (!result5.instruction->has_argument()) return false;
    if (!result5.instruction->has_reg_argument()) return false;
    if (result5.instruction->reg_argument() != Register::Imm) return false;
    if (!result5.instruction->has_immediate()) return false;
    if (result5.instruction->immedidate() != 0xab) return false;
    if (result5.instruction->encode() != data[4]) return false;
    auto result6 = Instruction::disassemble(result5.continueFrom);
    if (!result6) return false;
    if (!result6.instruction.has_value()) return false;
    if (result6.instruction->opcode() != OpCode::POP) return false;
    if (!result6.instruction->has_argument()) return false;
    if (!result6.instruction->has_reg_argument()) return false;
    if (result6.instruction->reg_argument() != Register::R1) return false;
    if (result6.instruction->encode() != data[6]) return false;
    auto result7 = Instruction::disassemble(result6.continueFrom);
    if (result7) return false;
    if (result7.instruction.has_value()) return false;
    if (result7.reason != FailureReason::InvalidArgument) return false;
    auto result8 = Instruction::disassemble(result7.continueFrom);
    if (result8) return false;
    if (result8.instruction.has_value()) return false;
    if (result8.reason != FailureReason::InvalidOpCode) return false;
    auto result9 = Instruction::disassemble(result8.continueFrom);
    if (result9) return false;
    if (result9.instruction.has_value()) return false;
    if (result9.reason != FailureReason::NoData) return false;

    ArgKind kind;
    return 
        opcode_has_arg(OpCode::LDA_CSR) == false &&
        opcode_has_arg(OpCode::POP, &kind) == true && kind == ArgKind::RegOnly &&
        true;
}

auto assemble_all(std::span<daisa::Instruction const> insns) {
    using namespace daisa;
    std::vector<std::array<u8, 256>> segments;
    auto result = assemble_segment(insns);
    while (result.has_remaining()) {
        segments.push_back(result.output);
        result = assemble_segment(result);
    }
    segments.push_back(result.output);
    return segments;
}

bool assemble_blocks_test() {
    using namespace daisa;
    constexpr auto insns = std::array{
        *Instruction::create(OpCode::SHL),
        *Instruction::create(OpCode::DEC, Register::R1),
        *Instruction::create(OpCode::Jc, (Condition)1/*TBD*/, 0),
        *Instruction::create(OpCode::RET)
    };
    auto segments = assemble_all(insns);
    constexpr auto expectSegments = std::array<std::array<u8, 256>, 1>{
        std::array<u8, 256>{
            0b11000100, // SHL
            0b10110001, // DEC r1
            0b00011001, 0, // Jnz(TBD) 0
            0b11000001 // RET
        }
    };
    constexpr auto constexprSeg = assemble_segment(insns).output;

    if (constexprSeg != expectSegments[0]) return false;
    if (segments.size() != expectSegments.size()) return false;
    for (auto i = 0u; i < segments.size(); i++) {
        if (segments[i] != expectSegments[i]) return false;
    }

    return true;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << argv[0] << " takes one argument.\n";
        return 1;
    }
    if (std::string(argv[1]) == "instruction")
        return !instruction_test();
    if (std::string(argv[1]) == "assemble_blocks")
        return !assemble_blocks_test();

    std::cout << "Unrecognized test." << std::endl;
    return 0;
}