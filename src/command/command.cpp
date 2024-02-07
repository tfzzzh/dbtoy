#include <command.h>
#include <cstdlib>
#include <iostream>
using std::cin;
using std::cout;
using std::endl;

void * Exit::evaluate() {
    exit(EXIT_SUCCESS);
    return nullptr;
}

void * Select::evaluate() {
    cout << "evaluate select" << endl;
    return nullptr;
}

void * Insert::evaluate() {
    cout << "evaluate insert" << endl;
    return nullptr;
}

Command * parse(const std::string & cmd) {
    if (cmd == ".exit") {
        return new Exit();

    } else if (cmd == "select") {
        return new Select();

    } else if (cmd == "insert") {
        return new Insert();
        
    } else {
        std::cout << "Unrecognized command '" << cmd << "'." << std::endl;
        return nullptr;
    }
}