//
// Created by Aincvy(aincvy@gmail.com) on 2022/02/26.
//

#pragma once

#include <system_error>
#include <memory>
#include <cstring>
#include "imterm/misc.hpp"
#include "imterm/terminal.hpp"
#include "imterm/terminal_helpers.hpp"

using namespace ImTerm;

namespace sight {

    struct TerminalRuntimeArgs {
        bool shouldClose = false;
    };

    class TerminalCommands : public ImTerm::basic_terminal_helper<TerminalCommands, TerminalRuntimeArgs> {
    public:
        TerminalCommands();

        static std::vector<std::string> no_completion(argument_type&) {
            return {};
        }

        static void clear(argument_type&);
        static void configure_term(argument_type&);
        static std::vector<std::string> configure_term_autocomplete(argument_type&);
        static void echo(argument_type&);
        static void exit(argument_type&);
        static void help(argument_type&);
        static void quit(argument_type&);

    };

    

}
