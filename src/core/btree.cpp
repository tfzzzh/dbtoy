#include "btree.h"
#include <cassert>
#include <memory>
#include <stdexcept>
#include <_types/_uint64_t.h>
using namespace std;
// class BPlusTree {
// public:
//     uint64_t get_root_page() const;
//     uint64_t get_total_page() const;
//     // void visit_rows(std::function<Row*()> handler);
//     // const Row* find(uint32_t key) const;
//     void insert(uint32_t, Row* rows);

// private:
//     BTreePager pager;
//     int leaf_load;
//     int inner_node_load;
// };
// [TODO]: write reconstruction for allocated tree nodes
// check: allocated node shall be deleted if not used
// check: set num_max_cell for debug btree
BPlusTree::BPlusTree(const string & path, char mode, uint32_t rsize, uint32_t leaf_node_load, uint32_t inner_node_load)
    : pager(path, mode), leaf_load(leaf_node_load), inner_node_load(inner_node_load), row_size(rsize)
{
    // when start from empty tree one must init the root page to a leaf node
    if (mode == 'c')
    {
        auto pid = pager.allocate_page(root_page);
        root = unique_ptr<LeafNode>(new LeafNode(root_page, row_size));

        // set parent of root to invalid
        root->parent() = NODE_PARENT_INVALID;
    }
    // when the tree is not first open we load its page from disk
    else
    {
        // warning uint64 to int
        root_page = pager.get_page(get_root_page());

        // build root node according to its type
        auto node_type = BtreeNode::get_node_type_from(root_page);
        switch (node_type)
        {
            case NODE_TYPE_INNER:
                root = unique_ptr<InternalNode>(new InternalNode(root_page, false));
                break;

            case NODE_TYPE_LEAF:
                root = unique_ptr<LeafNode>(new LeafNode(root_page));
                break;

            default:
                throw std::runtime_error("Unknown Node Type");
                break;
        }
    }

    // check root bit
    assert(root->is_root());
    assert(root->get_num_keys() > 0);
}

uint64_t BPlusTree::get_root_page() const
{
    uint64_t root_page = pager.get_root_page();
    // logic check: root_page is correct
    return root_page;
}

uint64_t BPlusTree::get_total_page() const
{
    return pager.num_pages();
}

// assume: key is not duplicated
void BPlusTree::insert(uint32_t key, Row * rows)
{
}