#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <memory>

namespace _log {
    char buf[512];

    std::string app_name;

    std::ofstream file;

    namespace type {
        #ifdef LOG_TARGET_LINUX
        const char *none    = "\u001b[30;1m[.]",
                   *debug   = "\u001b[34m[d]",
                   *ok      = "\u001b[32;1m[+]",
                   *info    = "\u001b[34;1m[i]",
                   *warning = "\u001b[35;1m[w]",
                   *error   = "\u001b[31;1m[e]";
        #else
            #ifdef LOG_TARGET_POWERSHELL
        const char *none    = "[.]",
                   *debug   = "[d]",
                   *ok      = "[+]",
                   *info    = "[i]",
                   *warning = "[w]",
                   *error   = "[e]";
            #else
        const char *none    = "[.]",
                   *debug   = "[d]",
                   *ok      = "[+]",
                   *info    = "[i]",
                   *warning = "[w]",
                   *error   = "[e]";
            #endif
        #endif
    }

    bool el = true;

    template <class... Args> void log(const char* t, std::string fmt, Args... args) {
        sprintf(buf, fmt.c_str(), args...);

        #ifdef LOG_TARGET_LINUX
        std::cout << t << "\u001b[0m " + app_name + ": " << buf << (el ? "\n" : "");
        #elif LOG_TARGET_POWERSHELL
        std::cout << t << app_name << ": " << buf << (el ? "\n" : "");
        #else
        std::cout << app_name + ": " << buf << (el ? "\n" : "");
        #endif

        if (file.is_open()) {
            std::string tstr(t), l = tstr.substr(tstr.find_last_of('['), 3) + " ";
            file << l << buf << std::endl;
        }
    }

    void init(std::string app, const std::string& fn = "", bool endline = true) {
        el = endline;
        app_name = app;
        if (fn.size()) {
            file.open(fn);
            if (!file.is_open()) {
                log(type::warning, "Couldn't open log file \"%s\"", fn.c_str());
            }
        }
    }
}

#define _log(t, ...) _log::log(_log::type::t, __VA_ARGS__)