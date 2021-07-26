#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <thread>

#include "sight.h"
#include "sight_ui.h"
#include "sight_js.h"
#include "sight_node_editor.h"

using namespace sight;

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 30);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    sight::exitSight(1);
}


int main(int argc, char* argv[]){
    signal(SIGSEGV, handler);

    std::thread jsThread(sight::jsThreadRun, argv[0]);

    sight::initTestData();
    sight::showMainWindow();

    jsThread.join();
    return 0;
}