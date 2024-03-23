#include "btree.h"
#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <queue>
#include <stdexcept>
#include "dbfile.h"
using namespace std;


unique_ptr<BtreeNode> BtreeNode::LoadNodeFrom(void * page)
{
    // build root node according to its type
    auto node_type = get_node_type_from(page);
    std::unique_ptr<BtreeNode> node;
    switch (node_type)
    {
        case NODE_TYPE_INNER:
            node = make_unique<InternalNode>(page, false);
            break;

        case NODE_TYPE_LEAF:
            node = make_unique<LeafNode>(page);
            break;

        default:
            throw std::runtime_error("Unknown Node Type");
            break;
    }

    return node;
}


BPlusTree::BPlusTree(const string & path, char mode, uint32_t rsize, uint32_t leaf_node_load, uint32_t inner_node_load)
    : pager(path, mode), leaf_load(leaf_node_load), inner_node_load(inner_node_load), row_size(rsize)
{
    // when start from empty tree one must init the root page to a leaf node
    if (mode == 'c')
    {
        root = nullptr;
        auto pid = pager.allocate_page(root_page);
        // root = unique_ptr<LeafNode>(new LeafNode(root_page, row_size));
        auto leaf = make_unique<LeafNode>(root_page, row_size);

        // set parent of root to invalid
        leaf->parent() = NODE_PARENT_INVALID;
        update_root(pid);
    }
    // when the tree is not first open we load its page from disk
    else
    {
        // warning uint64 to int
        root_page = pager.get_page(get_root_page());
        root = BtreeNode::LoadNodeFrom(root_page);
    }

    // check root bit
    assert(root->is_root());
    // assert(root->get_num_keys() > 0);
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

void BPlusTree::update_root(uint64_t page_id)
{
    // change bit of old root
    if (root != nullptr)
        root->set_root(false);

    pager.set_root_page(page_id);
    root_page = pager.get_page(page_id);
    root = BtreeNode::LoadNodeFrom(root_page);
    root->set_root(true);
    root->parent() = NODE_PARENT_INVALID;
}

// assume: key is not duplicated
BPlusTree::InsertStatus BPlusTree::insert(uint32_t key, Row * row)
{
    // find the leaf page to insert current key and row
    auto keyLocation = find(key);

    // handle the case: key duplicated use status
    if (keyLocation.is_exist)
        return InsertStatus::FAIL_DUPLICATE_KEY;

    // try into insert the key to the leaf
    auto page_id = keyLocation.page_id;
    LeafNode leaf(pager.get_page(page_id));
    leaf.set_node_load(min(leaf.num_max_cell, leaf_load));

    if (!leaf.isFull())
    {
        leaf.insert(key, row);
        return InsertStatus::SUCCESS;
    }

    // handle the case of leaf overflow
    void * new_page = nullptr;
    auto new_page_id = pager.allocate_page(new_page);
    auto key_upward = leaf.insert_and_split(key, row, new_page);
    // auto tmp = get_node_by(new_page_id);

    // post the overflow on inner node
    // now one shall insert (key_upward, left, right) to its parent node
    auto left = page_id;
    auto right = new_page_id;
    auto parent = leaf.parent();
    bool jobDone = false;
    while (parent != NODE_PARENT_INVALID && !jobDone)
    {
        InternalNode parentNode(pager.get_page(parent));
        parentNode.set_node_load(min(parentNode.num_max_keys, inner_node_load));
        // if parent node not full
        if (!parentNode.isFull())
        {
            parentNode.insert(key_upward, left, right);

            // link both left and right's parent to current parentNode
            // link_to(left, parent);
            assert(get_node_by(left)->parent() == parent);
            link_to(right, parent);

            jobDone = true;
        }
        // else
        else
        {
            new_page = nullptr;
            new_page_id = pager.allocate_page(new_page);
            // when inner node splits, the parent link shall move too
            auto pivot = parentNode.insert_and_split(key_upward, left, right, new_page);
            if (key_upward < pivot)
            {
                link_to(left, parent);
                link_to(right, parent);
            }
            else if (key_upward > pivot)
            {
                link_to(left, new_page_id);
                link_to(right, new_page_id);
            }
            // key_upard == pivot
            else
            {
                link_to(left, parent);
                link_to(right, new_page_id);
            }

            // reconfigure link of right page
            auto newRightNode = get_node_by(new_page_id);
            InternalNode* newRightNodeHandler = dynamic_cast<InternalNode*>(newRightNode.get());
            for (int r=0; r <= newRightNodeHandler->num_keys(); ++r)
            {
                auto child = get_node_by(newRightNodeHandler->get_child(r));
                child->parent() = new_page_id;
            }

            key_upward = pivot;
            left = parent;
            right = new_page_id;
            parent = parentNode.parent();
        }
    }

    // when root overflow one shall update the root
    // special case: root is a leafnode
    // root overflow and upsert (key_upward, left, right)
    if (!jobDone)
    {
        // assert()
        // new an empty root node
        new_page = nullptr;
        new_page_id = pager.allocate_page(new_page);
        InternalNode new_root(new_page, true);

        // insert (key_upward, left, right) to new root
        new_root.insert(key_upward, left, right);
        link_to(left, new_page_id);
        link_to(right, new_page_id);

        // change root of the tree
        update_root(new_page_id);
    }

    return InsertStatus::SUCCESS;
}

void BPlusTree::link_to(uint64_t child, uint64_t parent)
{
    auto childNode = BtreeNode::LoadNodeFrom(pager.get_page(child));
    childNode->parent() = parent;
}

KeyLocation BPlusTree::find(uint32_t key)
{
    uint64_t page_id = pager.get_root_page();
    auto curr = BtreeNode::LoadNodeFrom(root_page);

    // special case: root is empty
    if (curr->get_num_keys() == 0)
        return KeyLocation(page_id, 0, false);

    // search correct page id on leaf
    while (curr->node_type() != NODE_TYPE_LEAF)
    {
        // keys[pos] <= key < keys[pos+1]
        auto pos = curr->search_key_position(key);

        // when key == keys[pos] ->  search slot[pos]
        // when keys[pos] < key  -> search slot[pos+1]
        int slot = (pos >= 0 && curr->get_key(pos) == key) ? pos : pos + 1;

        // search next page
        page_id = ((InternalNode *)curr.get())->get_child(slot);
        curr = BtreeNode::LoadNodeFrom(pager.get_page(page_id));
    }

    // now current node is leaf
    assert(curr->node_type() == NODE_TYPE_LEAF);
    auto pos = curr->search_key_position(key);

    // find the key
    if (pos >= 0 && curr->get_key(pos) == key)
        return KeyLocation(page_id, pos, true);
    // key not found
    else
        return KeyLocation(page_id, pos + 1, false);
}

struct QueNodeInfo
{
    uint64_t page_id;
    unique_ptr<BtreeNode> node;
    int level;

    QueNodeInfo(uint64_t pid, unique_ptr<BtreeNode> && node_ptr, int level) : page_id(pid), node(std::move(node_ptr)), level(level) { }
};

void BPlusTree::print_keys()
{
    auto node_curr = BtreeNode::LoadNodeFrom(root_page);
    uint64_t page_id_curr = get_root_page();
    int level = -1;
    queue<QueNodeInfo> que;
    que.push({page_id_curr, std::move(node_curr), 0});

    while (!que.empty())
    {
        node_curr = std::move(que.front().node);
        int level_curr = que.front().level;
        page_id_curr = que.front().page_id;
        que.pop();

        if (level < level_curr)
        {
            level = level_curr;
            cout << "level " << level << ":" << endl;
        }

        printf("page %3lld: ", page_id_curr);
        // print key of the node
        for (int i = 0; i < node_curr->get_num_keys(); ++i)
            printf("%d ", node_curr->get_key(i));
        cout << endl;

        if (node_curr->node_type() == NODE_TYPE_INNER)
        {
            InternalNode * ptr = (InternalNode *)node_curr.get();
            for (int i = 0; i <= ptr->get_num_keys(); ++i)
            {
                auto page_id_next = ptr->get_child(i);
                void * page_next = pager.get_page(page_id_next);
                que.push({page_id_next, BtreeNode::LoadNodeFrom(page_next), level + 1});
            }
        }
    }
}


void BPlusTree::post_order_visit(
    uint64_t page_id,
    std::function<void(uint64_t)> inner_node_action,
    std::function<void(uint64_t)> leaf_node_action,
    std::function<void(void *)> leaf_cell_action,
    uint32_t min_key,
    uint32_t max_key)
{
    // when range is empty or node is empty, noting to do and return
    if (min_key > max_key)
        return;
    auto node = get_node_by(page_id);
    if (node->get_num_keys() == 0)
        return;

    int n = node->get_num_keys();

    // when node is a inner node. visit all its child then itself
    if (node->node_type() == NODE_TYPE_INNER)
    {
        int i = 0, j = n - 1;
        // loop until min_key <= node->get_key(i)
        while (i < n && node->get_key(i) < min_key)
            i += 1;

        // loop until key[j] < max_key
        while (j >= 0 && node->get_key(j) >= max_key)
            j -= 1;

        assert(i <= j + 1);

        // visit child node
        InternalNode * ptr = dynamic_cast<InternalNode *>(node.get());
        for (int k = i; k <= j + 1; ++k)
            post_order_visit(ptr->get_child(k), inner_node_action, leaf_node_action, leaf_cell_action, min_key, max_key);

        // visit current node
        if (inner_node_action != nullptr)
            inner_node_action(page_id);
    }
    // when current node is leaf, visit all row with key in range
    else
    {
        // visit row
        LeafNode * ptr = dynamic_cast<LeafNode *>(node.get());

        if (leaf_cell_action != nullptr)
        {
            for (int k = 0; k < n; ++k)
            {
                uint32_t key = ptr->get_key(k);
                if (key >= min_key && key <= max_key)
                    leaf_cell_action(ptr->get_cell(k));
            }
        }

        if (leaf_node_action != nullptr)
            leaf_node_action(page_id);
    }
}

bool BPlusTree::check_valid()
{
    // keys hall increasing in all nodes
    // if current node is root its root bit must be set, its parent is invalid
    // if current node is not root it root bit must unset, its loading shall >= 50%
    // if current node is internal, its childs's parents shall correctly set
    // if current node is internal, its
    // if current node is internal, max(lef_child) <= key < min(right_child)
    // return true;
    bool is_valid = true;
    std::function<void(uint64_t)> inner_node_checker = [this, &is_valid](uint64_t page_id)
    {
        // cout << page_id << endl;
        if (!is_valid) return;
        auto ptr = BtreeNode::LoadNodeFrom(pager.get_page(page_id));
        if (page_id == pager.get_root_page())
            is_valid = is_valid && (ptr->is_root()) && ptr->parent() == NODE_PARENT_INVALID;
        else
            is_valid = is_valid && (!ptr->is_root()) && ptr->parent() != NODE_PARENT_INVALID;

        if (!is_valid){
            cout << "root not correctly set" << endl;
            throw std::runtime_error("root not correctly set");
        }

        if (!ptr->is_root())
            is_valid = is_valid && ptr->get_num_keys() * 2 >= this->inner_node_load;
        is_valid = is_valid && ptr->is_key_monotonic_increasing();
        if (!is_valid) throw std::runtime_error("load or key increasing not satisfied");

        auto inner_ptr = (InternalNode *) ptr.get();
        for (uint32_t i=0; i < inner_ptr->get_num_keys(); ++i)
        {
            uint32_t key = inner_ptr->get_key(i);
            auto lchild = BtreeNode::LoadNodeFrom(pager.get_page(inner_ptr->get_child(i)));
            auto rchild = BtreeNode::LoadNodeFrom(pager.get_page(inner_ptr->get_child(i+1)));

            is_valid = is_valid && lchild->parent() == page_id;
            is_valid = is_valid && rchild->parent() == page_id;
            if (!is_valid) {
                cout << "left parent: " << lchild->parent() << " right parent" << rchild->parent() << " real " << page_id << endl;
                throw std::runtime_error("parent and child not correctly linked");
            }

            is_valid = is_valid && lchild->get_key(lchild->get_num_keys()-1) <= key;
            is_valid = is_valid && key < rchild->get_key(0);

            if (!is_valid) throw std::runtime_error("left child <= key < right child invalidate");
        }

        if (!is_valid) throw std::runtime_error("node is not valid");
    };

    // cout << pager.get_root_page() << endl;
    post_order_visit(pager.get_root_page(), inner_node_checker, nullptr, nullptr, 0, UINT32_MAX);
    return is_valid;
}

std::vector<void *> BPlusTree::select_cell(uint32_t min_val, uint32_t max_val)
{
    vector<void *> result;
    function<void(void *)> leaf_cell_action = [&result](void * cell) { result.push_back(cell); };
    post_order_visit(pager.get_root_page(), nullptr, nullptr, leaf_cell_action, min_val, max_val);
    return result;
}