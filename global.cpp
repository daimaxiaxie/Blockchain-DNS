//
// Created by lenovo on 2022/1/16.
//

#include "global.h"

bool EXIT = false;

void exit_handler(int sig) {
    EXIT = true;
}