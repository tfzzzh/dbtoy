#include <row.h>
#include <table.h>
#include <command.h>
#include <cstdlib>
#include <iostream>
using std::cin;
using std::cout;
using std::endl;

ExecuteResult * Exit::evaluate() {
    exit(EXIT_SUCCESS);
    return nullptr;
}

ExecuteResult * Select::evaluate() {
    // visit each row in memory
    UserInfo row;
    Table & table = TableBuffer::get_instance();

    for (int i=0; i < table.num_rows; ++i) {
        void * src = table.get_row_slot(i);
        row.deserialize(src);

        // print row
        std::cout << row.to_string() << std::endl;
    }

    return new ExecuteResult(ExecuteStatus::EXECUTE_SUCCESS);
}

Insert::Insert(const std::string & payload){
    row_to_insert = new UserInfo();
    row_to_insert->from_string(payload);
}

Insert::~Insert(){
    delete row_to_insert;
    row_to_insert = nullptr;
}

ExecuteResult * Insert::evaluate() {
    // cout << "evaluate insert" << endl;
    Table & table = TableBuffer::get_instance();

    if (table.num_rows < table.get_max_rows()) {
        void * mem = table.get_row_slot(table.num_rows);
        row_to_insert->serialize(mem);
        table.num_rows += 1;

        return new ExecuteResult(ExecuteStatus::EXECUTE_SUCCESS);
    } else {
        return new ExecuteResult(ExecuteStatus::TABLE_FULL);
    }
}

Command * parse(const std::string & cmd) {
    if (cmd == ".exit") {
        return new Exit();

    } else if (cmd == "select") {
        return new Select();

    // insert 1 cstack foo@bar.com
    } else if (cmd.substr(0, 6) == "insert") {
        return new Insert(cmd.substr(6, cmd.size() - 6));

    } else {
        std::cout << "Unrecognized command '" << cmd << "'." << std::endl;
        return nullptr;
    }
}