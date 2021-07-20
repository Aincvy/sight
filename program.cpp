#include <stdio.h>

#include "sight_ui.h"
#include "sight_js.h"

int main(int argc, char* argv[]){
    printf("123");
    sight::testJs(argv[0]);
    // sight::testWindow();
    return 0;
}