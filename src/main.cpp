#include <iostream>
#include <string>
#include <cstdlib>
#include <command/command.h>
#include <command/row.h>
#include <command/table.h>

using std::cin;
using std::cout;
using std::endl;

void prompt() {
    cout << "db > ";
}

// void test_row() {
//     std::string user = "elice";
//     std::string email = "elice@google.com";

//     UserInfo row(13, user.c_str(), email.c_str());

//     void* mem = malloc(sizeof(row.get_row_byte()));
//     cout << "here" << endl;

//     row.serialize(mem);

//     UserInfo other;
//     other.deserialize(mem);

//     cout << other.to_string() << endl;

//     free(mem);
// }

void test_table() {
    std::string user = "elice";
    std::string email = "elice@google.com";

    UserInfo row(13, user.c_str(), email.c_str());

    Table tab(row.get_row_byte());

    cout << "start" << endl;
    int stride = 500;
    for (int i=0; i < 5; ++i) {
        void * slot = tab.get_row_slot(i*stride);
        row.serialize(slot);
    }

    UserInfo row_new;
    for (int i=0; i < 5; ++i) {
        void * slot = tab.get_row_slot(i*stride);
        row_new.deserialize(slot);
        cout << row_new.to_string() << endl;
    }
}


int main(int argc, char* argv[]) {
    // test_row();
    test_table();

    while (true) {
        prompt();

        // get iuput from user
        std::string cmd;
        std::getline(cin, cmd);
        Command * query = parse(cmd);

        if (query != nullptr) {
            void * result = query->evaluate();
            delete query;
        } 
        // parse command
        // if (cmd == ".exit") {
        //     exit(EXIT_SUCCESS);

        // } else {
        //     cout << "Unrecognized command '" << cmd << "'." << endl;
        // }
    }

    return 0;
}