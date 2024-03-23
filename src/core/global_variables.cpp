#include "global_variables.h"

GlobalVariableHandler & GlobalVariableHandler::get_instance()
{
    static GlobalVariableHandler handler;
    return handler;
}

void GlobalVariableHandler::set_btree_paramters(
    size_t rsize, char mode_, const std::string & db_path, uint32_t leaf_node, uint32_t inner_node)
{
    row_size = rsize;
    path = db_path;
    leaf_load_upper_bound = leaf_node;
    inner_node_load_upper_bound = inner_node;
    mode = mode_;
}

BPlusTree & GlobalVariableHandler::get_btree()
{
    static BPlusTree btree(path, mode, row_size, leaf_load_upper_bound, inner_node_load_upper_bound);
    return btree;
}
