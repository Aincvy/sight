#include "sight.h"
#include "sight_ui.h"
#include "sight_js.h"
#include "sight_project.h"
#include "sight_util.h"
#include "sight_network.h"
#include "sight_log.h"
#include "sight_render.h"

#include <signal.h>
#include <thread>
#include <filesystem>

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
    logDebug("program start!");

#ifdef SIGHT_DEBUG
    backward::SignalHandling sh;
    logDebug("in debug mode");
    auto path = std::filesystem::current_path();
    auto cwd = path.generic_string();
    logDebug(cwd);
    if (endsWith(cwd, "/sight/build")) {
        if (path.has_parent_path()) {
            std::filesystem::current_path(path.parent_path());
            logDebug("change working directory: $0", std::filesystem::current_path().generic_string());
        }
    }
#endif
    signal(SIGSEGV, handler);

    logDebug(argv[0]);
    logDebug("load settings");
    loadSightSettings(nullptr);

    logDebug("init project");
    auto settings = getSightSettings();
    if (!settings->lastOpenProject.empty()) {
        openProject(settings->lastOpenProject.c_str(), true);
    } else {
        // 
        logDebug("no path, do not load project.");
    }

    logDebug("start js thread!");
    std::thread jsThread(sight::jsThreadRun, argv[0]);

    // logDebug("start network thread");
    // initNetworkServer(getSightSettings()->networkListenPort);
    // std::thread networkThread(sight::runNetworkThread);
    
    sight::initWindowBackend();
    sight::showLoadingWindow();
    if (currentProject() != nullptr) {
        sight::showMainWindow();
    }
    sight::destroyWindows();

    jsThread.join();
    return 0;
}