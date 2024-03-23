#pragma once
#include <string>
#include "row.h"

enum class ExecuteStatus
{
    EXECUTE_SUCCESS,
    EXECUTE_FAIL,
    DUPLICATE_KEY,
    TABLE_FULL
};

struct ExecuteResult
{
    ExecuteStatus status;
    void * data;

    ExecuteResult(ExecuteStatus status, void * data = nullptr) : status(status), data(data) { }
};

class Command
{
public:
    virtual ExecuteResult * evaluate() = 0;
    virtual ~Command(){};
};


class MetaCommand : public Command
{
};


class Exit : public MetaCommand
{
public:
    virtual ExecuteResult * evaluate() override;
};


class Statement : public Command
{
};


class Select : public Statement
{
public:
    virtual ExecuteResult * evaluate();
};

class SelectUsingBtree : public Select
{
public:
    virtual ExecuteResult * evaluate() override;
};


class Insert : public Statement
{
public:
    Insert(const std::string & payload);
    virtual ~Insert();
    virtual ExecuteResult * evaluate();

protected:
    Row * row_to_insert;
};

class InsertToBtree : public Insert
{
public:
    InsertToBtree(const std::string & payload) : Insert(payload) { }
    virtual ~InsertToBtree() override {};
    virtual ExecuteResult * evaluate() override;
};

Command * parse(const std::string & cmd);