#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <core/btree.h>
#include <core/command.h>
#include <core/global_variables.h>
#include <core/parameters.h>
#include <core/row.h>
#include <core/table.h>

using namespace std;


void prompt()
{
    cout << "db > ";
}

void initalize(const string & dbpath)
{
    char mode;
    if (std::filesystem::exists(dbpath))
        mode = 'o';
    else
        mode = 'c';

    auto & handler = GlobalVariableHandler::get_instance();
    handler.set_btree_paramters(UserInfo().get_row_byte(), mode, dbpath);
    handler.get_btree();
}

int main(int argc, char * argv[])
{
    // TableBuffer::row_size = UserInfo().get_row_byte();
    // TableBuffer::path = "/tmp/userinfo";
    initalize("/tmp/userinfo.db");

    while (true)
    {
        prompt();

        // get iuput from user
        std::string cmd;
        std::getline(cin, cmd);
        Command * query = parse(cmd);

        if (query != nullptr)
        {
            ExecuteResult * result = query->evaluate();

            switch (result->status) {
                case ExecuteStatus::DUPLICATE_KEY:
                    std::cout << "insert error: duplicate key, insertion ignored" << std::endl;
                    break;

                case ExecuteStatus::EXECUTE_FAIL:
                    std::cout << "execute command error, command ignored" << std::endl;
                    break;

                default:
                    break;
            }

            delete query;
            delete result;
        }
    }

    return 0;
}