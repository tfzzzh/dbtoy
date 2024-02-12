#pragma once
#include <string>
#include "row.h"

enum class ExecuteStatus {
    EXECUTE_SUCCESS,
    EXECUTE_FAIL,
    TABLE_FULL
};

struct ExecuteResult {
    ExecuteStatus status;
    void * data;

    ExecuteResult(ExecuteStatus status, void * data=nullptr):
        status(status), data(data) {}
};

class Command {
public:
    virtual ExecuteResult * evaluate() = 0;
    virtual ~Command() {};
};


class MetaCommand : public Command {};


class Exit: public MetaCommand {
public:
    virtual ExecuteResult * evaluate() override;
};


class Statement : public Command {};


class Select : public Statement {
public:
    virtual ExecuteResult * evaluate();
};


class Insert : public Statement {
public:
    Insert(const std::string & payload);
    virtual ~Insert();
    virtual ExecuteResult * evaluate();

private:
    Row * row_to_insert;
};


Command * parse(const std::string & cmd);