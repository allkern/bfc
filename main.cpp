#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>

#define BFC_TARGET_X64
#define BFC_COMPILER
#include "src/bfc.hpp"

#define LOG_TARGET_LINUX
#include "log.hpp"

#include "termios.h"
#include "unistd.h"

void __my_putchar(int i) {
    std::putchar(i);
}

int __my_getchar() {
    int c;  
 
    static termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO); 

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    c = std::getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (c == 27) {
        bfc::sigint_handler(0);
    }

    return c;
}

int main(int argc, const char* argv[]) {
    _log::init("bfc");

    std::ifstream file;

    if (!argv[1]) {
        _log(error, "No input files");

        std::exit(1);
    }

    file.open(argv[1]);

    if (!file.is_open()) {
        _log(error, "Couldn't open file \"%s\"", argv[1]);

        std::exit(1);
    }

    bfc::init(std::move(file));

    _log(info, "Compiling \"%s\"...", argv[1]);

    bfc::register_io_cb(__my_putchar, __my_getchar);

    bfc::compile();

    _log(ok, "Done!\nOutput: ");

    uint8_t* buf = bfc::get_buffer();

    uint8_t* ebuf = (uint8_t*)mmap(
        nullptr,
        bfc::get_buffer_size(),
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_ANON | MAP_PRIVATE,
        -1,
        0
    );

    std::memcpy(ebuf, buf, bfc::get_buffer_size());

    bfc::set_buffer(ebuf);

    for (size_t idx = 0; idx < bfc::get_buffer_size(); idx++) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)bfc::buf[idx] << " ";
    } std::cout << std::endl;

    bfc::execute();

    bfc::close();

    file.close();
    
    return 0;
}