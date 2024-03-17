#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <core/btree.h>
#include <core/command.h>
#include <core/row.h>
#include <core/table.h>
#include "core/parameters.h"

using namespace std;


void prompt()
{
    cout << "db > ";
}

struct TreeNode
{
    int page_id;
    vector<uint32_t> keys;
    vector<TreeNode *> children;
    bool isleaf;
};

struct NaryTree
{
    NaryTree(TreeNode * root = nullptr, int total_nodes = 0, uint32_t node_load = 4)
        : root(root), total_nodes(total_nodes), node_load(node_load)
    {
    }

    BPlusTree * clone(const string & path)
    {
        // BPlusTree(const std::string & path, char mode, uint32_t row_size, uint32_t leaf_node_load = 10, uint32_t inner_node_load = 10);
        BPlusTree * btree = new BPlusTree(path, 'c', UserInfo().get_row_byte(),
            node_load, node_load);

        // make the btree have the same pages with NaryTree
        for(size_t i=0; i < total_nodes; ++i) {
            void * page;
            btree->pager.allocate_page(page);
        }

        // foreach node connect it with its child
        auto visitor = [&btree, this](TreeNode * node) {
            auto page_id = node->page_id;
            if (!node->isleaf) {
                auto  btreeNode = make_unique<InternalNode>(
                    btree->pager.get_page(page_id),
                    true
                );

                for (int i=0; i < node->keys.size(); ++i)
                {
                    auto key = node->keys[i];
                    auto lchild = node->children[i];
                    auto rchild = node->children[i+1];
                    btreeNode->insert(key, lchild->page_id, rchild->page_id);

                    // set parent of child node
                    auto lnode = BtreeNode::LoadNodeFrom(
                        btree->pager.get_page(lchild->page_id)
                    );
                    auto rnode = BtreeNode::LoadNodeFrom(
                        btree->pager.get_page(rchild->page_id)
                    );

                    lnode->parent() = page_id;
                    rnode->parent() = page_id;
                }

            } else {
                auto  btreeNode = make_unique<LeafNode>(
                    btree->pager.get_page(page_id),
                    UserInfo().get_row_byte()
                );

                for (int i=0; i < node->keys.size(); ++i)
                {
                    btreeNode->allocate_cell();
                    btreeNode->get_key(i) = node->keys[i];
                }
            }

            if (node == this->root)
            {
                btree->pager.set_root_page(page_id);
                btree->root_page = btree->pager.get_page(page_id);
                btree->root = BtreeNode::LoadNodeFrom(btree->root_page);
                btree->root->set_root(true);
            }
        };

        // set root node
        post_order_visit(root, visitor);

        return btree;
    }

    static NaryTree * tree1()
    {
        vector<TreeNode *> nodes;

        // 8 leaf node
        for (uint32_t i = 0; i < 8; ++i)
        {
            TreeNode * node = new TreeNode;
            node->page_id = i;
            node->isleaf = true;
            node->keys = vector<uint32_t>({8 * i + 2, 8 * i + 4, 8 * i + 6, 8 * i + 8});
            nodes.push_back(node);
        }

        // 2 intermediate node
        TreeNode * node = new TreeNode;
        node->page_id = 8;
        node->isleaf = false;
        node->keys = {8u, 16u, 24u, 32u};
        node->children = {nodes[0], nodes[1], nodes[2], nodes[3], nodes[4]};
        nodes.push_back(node);

        node = new TreeNode;
        node->page_id = 9;
        node->isleaf = false;
        node->keys = {48u, 56u};
        node->children = {nodes[5], nodes[6], nodes[7]};
        nodes.push_back(node);

        // root
        node = new TreeNode;
        node->page_id = 10;
        node->isleaf = false;
        node->keys = {40u};
        node->children = {nodes[8], nodes[9]};
        nodes.push_back(node);

        return new NaryTree(nodes.back(), 11);
    }

    void post_order_visit(TreeNode * node, function<void(TreeNode *)> action)
    {
        if (node->isleaf)
        {
            action(node);
            return;
        }

        // visit all its children
        for (auto child : node->children)
            post_order_visit(child, action);

        action(node);
    }

    // print the tree level by level
    void printTree()
    {
        if (root == nullptr)
        {
            cout << "tree is empty" << endl;
            return;
        }

        queue<pair<TreeNode *, int>> que;
        que.push({root, 0});
        int level = -1;

        while (!que.empty())
        {
            auto front = que.front();
            que.pop();

            if (level < front.second)
            {
                level = front.second;
                cout << "level " << level << ":" << endl;
            }

            // print the node
            printf("nodeid %3d: ", front.first->page_id);
            // print key of the node
            for (auto key : front.first->keys)
                printf("%d ", key);
            cout << endl;

            for (TreeNode * child : front.first->children)
                que.push({child, level + 1});
        }
    }

    ~NaryTree()
    {
        if (root != nullptr)
        {
            auto deleter = [](TreeNode * node) { delete node; };
            post_order_visit(root, deleter);
        }
    }

    TreeNode * root;
    uint32_t total_nodes;
    uint32_t node_load;
};


void test_nary_tree()
{
    auto tree = NaryTree::tree1();
    tree->printTree();
}

void test_clone_nary_tree()
{
    auto tree = NaryTree::tree1();
    string path = "/tmp/narytree";
    BPlusTree * btree = tree->clone(path);
    btree->print_keys();
    delete btree;
}

void test_load_btree()
{
    string path = "/tmp/narytree";
    BPlusTree * btree = new  BPlusTree(path, 'o', UserInfo().get_row_byte(), 4, 4);
    btree->print_keys();
    delete btree;
}

void test_find()
{
    string path = "/tmp/narytree";
    BPlusTree * btree = new  BPlusTree(path, 'o', UserInfo().get_row_byte(), 4, 4);
    btree->print_keys();

    // 8 * i + 2
    for (int key=2; key <=64; key += 2)
    {
        auto result = btree->find(key);
        assert(result.is_exist);
        assert(result.page_id == (key-2) / 8);
        // 2,4,6,8 -> (0,1,2,3)
        // 10,12,14,16 -> (0,1,2,3)
        // 18,20,22,24 -> (0,1,2,3) (x-2) % 4
        assert(result.row_id == (key-2)/2%4);
    }

    for (int key=2; key <=64; key += 2)
    {
        auto result = btree->find(key);
        assert(result.is_exist);
        assert(result.page_id == (key-2) / 8);
        // 2,4,6,8 -> (0,1,2,3)
        // 10,12,14,16 -> (0,1,2,3)
        // 18,20,22,24 -> (0,1,2,3) (x-2) % 4
        assert(result.row_id == (key-2)/2%4);
    }

    // (1,3,5,7) -> page 0 pos (0,1,2,3)
    // (9,11,13,15) -> page 1 pos (0,1,2,3)
    // ...
    // (57,59,61,63) -> page 7
    for (int key=1; key <=63; key += 2)
    {
        auto result = btree->find(key);
        assert(!result.is_exist);
        assert(result.page_id == key / 8);
        // 2,4,6,8 -> (0,1,2,3)
        // 10,12,14,16 -> (0,1,2,3)
        // 18,20,22,24 -> (0,1,2,3) (x-2) % 4
        assert(result.row_id == (key-1)/2%4);
    }

    // key == 65
    for (int key=65; key <=120; key += 10)
    {
        auto result = btree->find(key);
        assert(!result.is_exist);
        assert(result.page_id == 7);
        // 2,4,6,8 -> (0,1,2,3)
        // 10,12,14,16 -> (0,1,2,3)
        // 18,20,22,24 -> (0,1,2,3) (x-2) % 4
        assert(result.row_id == 4);
    }

    delete btree;
}

void test_sequence_insert()
{
    BPlusTree * btree = new  BPlusTree("/tmp/test_sequence_insert", 'c', UserInfo().get_row_byte(), 4, 4);
    int n = 103;

    for (int i=0; i < n; ++i) {
        string name = to_string(i);
        UserInfo row(i, name.c_str(), name.c_str());
        btree->insert(i, &row);
    }

    btree->print_keys();
    delete btree;
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

// void test_table() {
//     std::string user = "elice";
//     std::string email = "elice@google.com";

//     UserInfo row(13, user.c_str(), email.c_str());

//     Table tab(row.get_row_byte());

//     cout << "start" << endl;
//     int stride = 500;
//     for (int i=0; i < 5; ++i) {
//         void * slot = tab.get_row_slot(i*stride);
//         row.serialize(slot);
//     }

//     UserInfo row_new;
//     for (int i=0; i < 5; ++i) {
//         void * slot = tab.get_row_slot(i*stride);
//         row_new.deserialize(slot);
//         cout << row_new.to_string() << endl;
//     }
// }


int main(int argc, char * argv[])
{
    TableBuffer::row_size = UserInfo().get_row_byte();
    TableBuffer::path = "/tmp/userinfo";
    // test_nary_tree();
    // test_clone_nary_tree();
    // test_load_btree();
    // test_find();
    test_sequence_insert();
    // auto page_ptr = malloc(PAGE_SIZE);
    // InternalNode inner(page_ptr, true);
    // cout << "max_num_keys: " << inner.num_max_keys << endl;
    // for (int i=0; i < inner.num_max_keys; ++i) {
    //     inner.get_key(i) = i;
    //     inner.set_child(i, nullptr);
    //     inner.num_keys += 1;
    // }
    // inner.set_child(inner.num_max_keys, nullptr);
    // inner.print_node();

    while (true)
    {
        prompt();

        // get iuput from user
        std::string cmd;
        std::getline(cin, cmd);
        Command * query = parse(cmd);

        if (query != nullptr)
        {
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