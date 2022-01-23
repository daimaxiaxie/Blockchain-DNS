#include <iostream>

#include "server.h"

int main(int argc, char *argv[]) {
    signal(SIGINT, exit_handler);
    const char *file = "config.json";
    if (argc > 1) {
        file = argv[1];
    }
    Config config(file);
    Server server(&config);
    server.init();
    server.run();
    return 0;
}
