#pragma once

#include <algorithm>
#include <iostream>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <queue>
#include <array>
#include <stack>

#include "emitter.hpp"
#include "io.hpp"
#include "ir.hpp"

#ifdef __linux__
#include <sys/mman.h>
#endif

namespace bfc {
    std::array <uint16_t, 0xffff> memory;

#ifdef BFC_INTERPRETER
    size_t ip = 0;
    uint16_t dp = 0;

    std::stack <size_t> stack;
    std::string code;

    size_t find_branch_pos(const std::string& code) {
        int cp = ip, counter = 1;

        while (counter) {
            char c = code[++cp];

            if (c == '[') {
                counter++;
            } else if (c == ']') {
                counter--;
            }
        }
        return cp;
    }

    void load_stream() {
        char buffer[8192];

        while (input->read(buffer, sizeof(buffer))) code.append(buffer, sizeof(buffer));
    
        code.append(buffer, input->gcount());
    }

    void execute() {
        while (ip < code.size()) {
            switch (code.at(ip)) {
                case '+': memory[dp]++; break;
                case '-': memory[dp]--; break;
                case '>': dp++; break;
                case '<': dp--; break;
                case '.': std::putchar(memory.at(dp)); break;
                case ',': memory[dp] = std::getchar(); break;
                case '[': { if (!memory[dp]) { ip = find_branch_pos(code); } else { stack.push(ip); } } break;
                case ']': if (memory[dp]) { ip = stack.top(); } else { stack.pop(); } break;
            }

            ip++;
        }
    }
#endif

#ifdef BFC_COMPILER
    uint8_t* buf = nullptr;

    void close() {
        input.release()->clear();
    }

    static void sigint_handler(int signal) {
        close();

        std::exit(1);
    }

    namespace backend {
        uint64_t fputchar_ptr = reinterpret_cast<uint64_t>(&std::putchar),
                 fgetchar_ptr = reinterpret_cast<uint64_t>(&std::getchar),
                 fmemory_ptr = reinterpret_cast<uint64_t>(memory.data());

        std::stack <size_t> fw_branch_stack;

        size_t buf_idx = 0;

        void push_buffer(uint64_t data, size_t size, size_t* idx_ptr = nullptr) {
            *reinterpret_cast<uint64_t*>(&buf[buf_idx]) = data;

            if (idx_ptr) (*idx_ptr) += size;

            buf_idx += size;
        }
    }

    typedef void (*putchar_callback_t)(int);
    typedef int (*getchar_callback_t)();
    
    void register_io_cb(putchar_callback_t new_putchar_cb, getchar_callback_t new_getchar_cb) {
        backend::fputchar_ptr = reinterpret_cast<uint64_t>(new_putchar_cb);
        backend::fgetchar_ptr = reinterpret_cast<uint64_t>(new_getchar_cb);
    }

    void register_io_cb(uint64_t new_putchar_cb, uint64_t new_getchar_cb) {
        backend::fputchar_ptr = new_putchar_cb;
        backend::fgetchar_ptr = new_getchar_cb;
    }

    void set_memory_buffer(void* new_memory_ptr) {
        delete[] memory.data();

        backend::fmemory_ptr = reinterpret_cast<uint64_t>(new_memory_ptr);
    }

    void set_memory_buffer(uint64_t new_memory_ptr) {
        delete[] memory.data();

        backend::fmemory_ptr = new_memory_ptr;
    }

    void compile() {
        using namespace backend;

        std::signal(SIGINT, sigint_handler);

        ir::generate();
        ir::optimize();

        push_buffer(fputchar_ptr, 8);
        push_buffer(fgetchar_ptr, 8);

        /*
            push   %rbp
            mov    %rsp,%rbp
            xor    %rax,%rax
            movabs %r12,$memory_base
        */
        push_buffer(bswapn(0x554889e5, 4), 4);
        push_buffer(bswapn(0x4831db, 3), 3);
        push_buffer(bswapn(0x49bc, 2), 2);
        push_buffer(fmemory_ptr, 8);

        size_t idx = backend::buf_idx;

        bool emit_getchar_return_transfer = false;

        while (!ir::empty()) {
            auto i = ir::get();

            emitter::return_t e;

            switch (i.opcode) {
                case ir::BFIR_ADD: { e = emitter::add(i.operand); } break;
                case ir::BFIR_SUB: { e = emitter::sub(i.operand); } break;
                case ir::BFIR_ADP: { e = emitter::adp(i.operand); } break;
                case ir::BFIR_SBP: { e = emitter::sdp(i.operand); } break;
                case ir::BFIR_PUT: { push_buffer(bswapn(0x498b3c5cul, 4), 4, &idx); e = emitter::call(-(idx+6)); } break;
                case ir::BFIR_GET: { e = emitter::call(8-(idx+6)); emit_getchar_return_transfer = true; } break;
                case ir::BFIR_BEQ: { e = emitter::beq(0); fw_branch_stack.push(idx); push_buffer(bswapn(0x6641833c5c00, 6), 6, &idx); } break;
                case ir::BFIR_BNE: {
                    size_t bw_branch_idx = fw_branch_stack.top();
                
                    e = emitter::bne((bw_branch_idx + 6) - (idx + 6));
                    
                    *reinterpret_cast<uint32_t*>(&buf[bw_branch_idx + 8]) = (idx + 6) - (bw_branch_idx + 6);

                    fw_branch_stack.pop();

                    push_buffer(bswapn(0x6641833c5c00, 6), 6, &idx);
                } break;
                case ir::BFIR_SET: { e = emitter::mov(i.operand); } break;
            }

            push_buffer(bswapn(e.data, e.size), e.size);

            if (emit_getchar_return_transfer) {
                push_buffer(bswapn(0x664189045c, 5), 5, &idx);
                emit_getchar_return_transfer = false;
            }

            idx += e.size;
        }

        /*
            leaveq
            retq
        */
        push_buffer(bswapn(0xc9c3, 2), 2);
    }

    void init(std::istream&& new_stream) {
        input.reset(&new_stream);

        buf = new uint8_t[0xffff];
    }

    void set_buffer(void* new_buf) {
        delete[] buf;

        buf = (uint8_t*)new_buf;
    }

    uint8_t* get_buffer() {
        return buf;
    }

    size_t get_buffer_size() {
        return backend::buf_idx;
    }

    void execute() {
        ((void(*)(void))(buf + 16))();
    }
#endif
}
