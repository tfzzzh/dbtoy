#pragma once
#include "btree.h"

class GlobalVariableHandler
{
public:
    static GlobalVariableHandler & get_instance();
    void set_btree_paramters(size_t rsize, char mode_, const std::string & db_path, uint32_t leaf_node = 10000, uint32_t inner_node = 1000);
    BPlusTree & get_btree();


private:
    GlobalVariableHandler() {};

    // parameteres for btree
    size_t row_size;
    std::string path;
    uint32_t leaf_load_upper_bound;
    uint32_t inner_node_load_upper_bound;
    char mode;
};