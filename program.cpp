#include <signal.h>
#include <thread>
#include <filesystem>

#include "dbg.h"
#include "sight.h"
#include "sight_ui.h"
#include "sight_js.h"
#include "sight_project.h"
#include "sight_util.h"

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
    dbg("in debug mode");
    auto path = std::filesystem::current_path();
    auto cwd = path.generic_string();
    dbg(cwd);
    if (endsWith(cwd, "/sight/build")) {
        if (path.has_parent_path()) {
            std::filesystem::current_path(path.parent_path());
            dbg("change working directory", std::filesystem::current_path().generic_string());
        }
    }
#endif
    signal(SIGSEGV, handler);

    initProject("./project", true);

    dbg("start js thread!");
    std::thread jsThread(sight::jsThreadRun, argv[0]);

    sight::initOpenGL();
    sight::showLoadingWindow();
    sight::showMainWindow();
    sight::destroyWindows();

    jsThread.join();
    return 0;
}