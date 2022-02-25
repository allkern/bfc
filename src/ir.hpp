#pragma once

#include "io.hpp"

namespace bfc {
    namespace ir {
        enum opcode_t : uint8_t {
            BFIR_ADD,
            BFIR_SUB,
            BFIR_ADP,
            BFIR_SBP,
            BFIR_PUT,
            BFIR_GET,
            BFIR_BEQ,
            BFIR_BNE,
            BFIR_SET
        };

        static const char* opcode[] = {
            "BFIR_ADD",
            "BFIR_SUB",
            "BFIR_ADP",
            "BFIR_SBP",
            "BFIR_PUT",
            "BFIR_GET",
            "BFIR_BEQ",
            "BFIR_BNE",
            "BFIR_SET"
        };

        struct instruction_t {
            opcode_t opcode;

            uint8_t operand;
        };

        char last = ' ';

        std::vector <instruction_t> ir;

        size_t idx;

        namespace opt {
            uint8_t chain() {
                uint8_t chain_size = 1;

                while (input->peek() == last) { input->get(); chain_size++; }

                return chain_size;
            }

            // Parses sequences of the form [-]+* into BFIR_SET(plus count)
            void set_seq(size_t idx) {
                if (!(ir.at(idx).opcode == BFIR_BEQ)) return;

                if ((ir.at(idx + 1).opcode == BFIR_SUB) &&
                    (ir.at(idx + 2).opcode == BFIR_BNE)) {
                    
                    ir.at(idx).opcode = BFIR_SET;
                    ir.at(idx).operand = 0;

                    ir.erase(std::begin(ir) + idx + 1, std::begin(ir) + idx + 3);
                }

                if ((idx + 1) >= ir.size()) return;

                if (ir.at(idx + 1).opcode == BFIR_ADD) {
                    ir.at(idx).operand = ir.at(idx + 1).operand;

                    ir.erase(std::begin(ir) + (idx + 1));
                }
            }

            opcode_t opposite(opcode_t o) { return (opcode_t)((int)o ^ 0x1); }

            void cancel(size_t idx) {
                opcode_t first = ir.at(idx).opcode;

                if ((int)first < 4) {
                    if ((idx + 1) >= ir.size()) return;

                    if (ir.at(idx + 1).opcode == opposite(first)) {
                        uint8_t f_op = ir.at(idx).operand,
                                s_op = ir.at(idx + 1).operand;

                        if (f_op > s_op) { ir.at(idx).operand = f_op - s_op; ir.erase(std::begin(ir) + idx + 1); }
                        if (f_op < s_op) { ir.at(idx).opcode = opposite(first); ir.at(idx).operand = s_op - f_op; ir.erase(std::begin(ir) + idx + 1); }
                        if (f_op == s_op) { ir.erase(std::begin(ir) + idx, std::begin(ir) + idx + 1); }
                    }
                }
            }
        }

        void generate() {
            while (!input->eof()) {
                switch (last = input->get()) {
                    case '+': ir.push_back({ BFIR_ADD, opt::chain() }); break;
                    case '-': ir.push_back({ BFIR_SUB, opt::chain() }); break;
                    case '>': ir.push_back({ BFIR_ADP, opt::chain() }); break;
                    case '<': ir.push_back({ BFIR_SBP, opt::chain() }); break;
                    case '.': ir.push_back({ BFIR_PUT, 1 }); break;
                    case ',': ir.push_back({ BFIR_GET, 1 }); break;
                    case '[': ir.push_back({ BFIR_BEQ, 1 }); break;
                    case ']': ir.push_back({ BFIR_BNE, 1 }); break;
                }
            }
        }

        void optimize() {
            // First stage
            for (size_t i = 0; i < ir.size(); i++) {
                opt::set_seq(i);
                opt::cancel(i);
            }
        }

        bool empty() {
            return idx == ir.size();
        }

        instruction_t get() {
            return ir.at(idx++);
        }
    }
}