#pragma once
#include <string>

class Command {
public:
    virtual void * evaluate() = 0;
    virtual ~Command() {};
};


class MetaCommand : public Command {};


class Exit: public MetaCommand {
public:
    virtual void * evaluate() override;
};


class Statement : public Command {};


class Select : public Statement {
public:
    virtual void * evaluate();
};


class Insert : public Statement {
public:
    virtual void * evaluate();
};


Command * parse(const std::string & cmd);