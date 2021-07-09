#include <daisa.hpp>
#include <iostream>

#include <string>

bool instruction_test() {
    using namespace daisa;
    return 
        opcode_has_arg(Opcode::LDA_CSR) == std::nullopt &&
        opcode_has_arg(Opcode::POP).value_or(static_cast<ArgKind>(-1)) == ArgKind::RegOnly &&
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