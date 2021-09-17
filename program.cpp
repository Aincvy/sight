#include <signal.h>
#include <thread>

#include "sight.h"
#include "sight_ui.h"
#include "sight_js.h"
#include "sight_node_editor.h"
#include "sight_project.h"

#ifdef SIGHT_DEBUG
#include "backward.hpp"
#endif

using namespace sight;

void handler(int sig) {
    if (sig == SIGABRT) {
        exitSight(1);
    }
}

int main(int argc, char* argv[]){
    dbg("program start!");

#ifdef SIGHT_DEBUG
    backward::SignalHandling sh;
#endif
    signal(SIGSEGV, handler);

    initProject("./project", true);

    dbg("start js thread!");
    std::thread jsThread(sight::jsThreadRun, argv[0]);

    sight::showMainWindow();

    jsThread.join();
    return 0;
}