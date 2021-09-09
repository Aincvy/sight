#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <thread>

#include "sight.h"
#include "sight_ui.h"
#include "sight_js.h"
#include "sight_node_editor.h"

#ifdef SIGHT_DEBUG
#define BACKWARD_HAS_BFD 1
#define BACKWARD_HAS_DW 1
#include "backward.hpp"
#endif

using namespace sight;

void handler(int sig) {

}

int main(int argc, char* argv[]){
    dbg("program start!");

#ifdef SIGHT_DEBUG
    backward::SignalHandling sh;
#endif
    signal(SIGSEGV, handler);

    dbg("start js thread!");
    std::thread jsThread(sight::jsThreadRun, argv[0]);

    sight::showMainWindow();

    jsThread.join();
    return 0;
}