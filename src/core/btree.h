#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <sys/_types/_int64_t.h>
#include "dbfile.h"
#include "parameters.h"
#include "row.h"

/**
 * @brief common header layer out
 * NODE_TYPE 1 byte
 * IS_ROOT 1 byte
 * PARENT_POINTER 8 byte
 */
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint64_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;
const uint64_t NODE_PARENT_INVALID = ULONG_LONG_MAX;
const uint8_t NODE_TYPE_INNER = 0;
const uint8_t NODE_TYPE_LEAF = 1;

struct BtreeNode
{
    void * data; // pointer to a page

    virtual const uint8_t & node_type() const
    {
        const uint8_t & value = *((uint8_t *)((char *)data + NODE_TYPE_OFFSET));
        return value;
    }

    virtual bool is_root() const
    {
        bool result = *((char *)data + IS_ROOT_OFFSET);
        return result;
    }

    virtual void set_root(bool is_root = true) { *((char *)data + IS_ROOT_OFFSET) = (int)is_root; }

    // when current node's parent is not valid set it to -1
    virtual uint64_t & parent() { return *(uint64_t *)((char *)data + PARENT_POINTER_OFFSET); }

    // get and set load of current node
    virtual uint32_t get_node_load() const { throw std::runtime_error("not implemented"); }

    virtual void set_node_load(uint32_t size) { throw std::runtime_error("not implemented"); }

    // get number of keys for a node
    virtual uint32_t get_num_keys() const { throw std::runtime_error("not implemented"); }

    // get key according to its id, one can use this
    // function to change key value
    virtual uint32_t & get_key(uint32_t key_id) { throw std::runtime_error("not implemented"); }

    // check whether current node is full
    virtual bool isFull() { throw std::runtime_error("not implemented"); }

    // search pos s.t. keys[pos] <= query_key < keys[pos+1]
    // pos == -1 if query_key < keys[0]
    // assumption: all keys in a nodes are distinct
    virtual int search_key_position(uint32_t key)
    {
        uint32_t n = get_num_keys();
        assert(n > 0);

        int start = 0, end = n - 1;
        while (start <= end)
        {
            int mid = start + (end - start) / 2;
            uint32_t key_mid = get_key(mid);

            if (key_mid == key)
                return mid;
            // key[start .. mid] < key
            else if (key_mid < key)
                start = mid + 1;
            // key < key_mid
            else
                end = mid - 1;
        }

        // keys[end] < key < keys[start]
        assert(start == end + 1);
        return end;
    }

    // check if the node contains duplicate keys
    virtual bool contain_duplicate()
    {
        uint32_t n = get_num_keys();

        for (uint32_t i = 0; i + 1 < n; ++i)
            if (get_key(i) == get_key(i + 1))
                return true;


        return false;
    }

    // static functions
    static uint8_t get_node_type_from(void * page)
    {
        const uint8_t & value = *((uint8_t *)((char *)page + NODE_TYPE_OFFSET));
        return value;
    }

    // factory function to genrate node according to content in page
    static std::unique_ptr<BtreeNode> LoadNodeFrom(void * page);

protected:
    // constructor shall not be called
    BtreeNode(void * page) : data(page)
    {
        // the page shall known
        // data = malloc(PAGE_SIZE);

        // one shall not change *page when it is the only parameter of
        // constructor
        // defaultly the node is not a root node
        // set_root(false);
        // parent() = NODE_PARENT_INVALID;
    }
};

/**
 * @brief header of leaf node layout
 * NUM_CELLS 4 byte
 * row_size 4 byte
 */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_ROWSIZE_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_ROWSIZE_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_ROWSIZE_SIZE;

/**
 * @brief layout of payload (key, value)
 *
 */
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;


/**
 * @brief leafnode of b+tree
 *
 */
struct LeafNode : public BtreeNode
{
    uint32_t row_size; // size of a row
    uint32_t cell_size; // concate of key and row
    uint32_t num_max_cell; // shall never change them

    LeafNode(void * page, uint32_t row_size) : BtreeNode(page), row_size(row_size)
    {
        cell_size = sizeof(uint32_t) + row_size;
        num_max_cell = LEAF_NODE_SPACE_FOR_CELLS / cell_size;

        // enforce that the load of tree is even
        if (num_max_cell % 2 == 1)
            num_max_cell -= 1;

        // set num_cell to 0
        num_cells() = 0;

        assert(num_max_cell > 0);

        // set node type
        *((uint8_t *)((char *)data + NODE_TYPE_OFFSET)) = NODE_TYPE_LEAF;

        // set row size
        *get_rowsize_ptr() = row_size;
    }

    // build node directly from page
    LeafNode(void * page) : BtreeNode(page)
    {
        row_size = *get_rowsize_ptr();

        cell_size = sizeof(uint32_t) + row_size;
        num_max_cell = LEAF_NODE_SPACE_FOR_CELLS / cell_size;

        // enforce that the load of tree is even
        if (num_max_cell % 2 == 1)
            num_max_cell -= 1;

        assert(num_max_cell > 0);
    }

    virtual uint32_t get_node_load() const override { return num_max_cell; }

    virtual void set_node_load(uint32_t size) override
    {
        assert(size % 2 == 0 && size > 0);
        num_max_cell = size;
    }

    virtual uint32_t get_num_keys() const override
    {
        uint32_t * num_cells_ptr = (uint32_t *)((char *)data + (size_t)LEAF_NODE_NUM_CELLS_OFFSET);
        return *num_cells_ptr;
    }

    // get pointer to row_size
    uint32_t * get_rowsize_ptr() { return (uint32_t *)((char *)data + LEAF_NODE_ROWSIZE_OFFSET); }

    // num of (key, value) pairs
    // note that :: add num_cells by one when new row inserted
    uint32_t & num_cells()
    {
        uint32_t * num_cells_ptr = (uint32_t *)((char *)data + (size_t)LEAF_NODE_NUM_CELLS_OFFSET);
        return *num_cells_ptr;
    }

    // pair of (key, val)
    // cell_num < leaf_node_num_cells
    void * get_cell(uint32_t cell_num)
    {
        assert(cell_num < num_cells());
        return (char *)data + LEAF_NODE_HEADER_SIZE + cell_num * cell_size;
    }

    // allocate new cell from the page
    // when no slot available return nullptr
    void * allocate_cell()
    {
        uint32_t m = num_cells();
        if (m == num_max_cell)
            return nullptr;

        void * pos = (char *)data + LEAF_NODE_HEADER_SIZE + m * cell_size;

        num_cells() += 1;
        return pos;
    }

    // get key of a cell
    virtual uint32_t & get_key(uint32_t cell_num) override { return *((uint32_t *)get_cell(cell_num)); }

    // pointer to value content, shall deserialize using row object
    void * get_value(uint32_t cell_num)
    {
        void * cell_ptr = get_cell(cell_num);
        return (char *)cell_ptr + LEAF_NODE_VALUE_OFFSET;
    }

    bool is_duplicate(uint32_t key)
    {
        for (uint32_t i = 0; i < num_cells(); ++i)
            if (get_key(i) == key)
                return true;
        return false;
    }

    // duplicate is not checked using this method
    // insert (key, value) into the page make the page sorted by key
    void insert(uint32_t key, Row * value)
    {
        assert(num_cells() < num_max_cell);
        auto m = num_cells();

        // allocate a place for the newly insert row
        allocate_cell();
        auto i = m;
        while (i >= 1 && get_key(i - 1) > key)
        {
            // copy cell[i-1] to cell[i]
            memcpy(get_cell(i), get_cell(i - 1), cell_size);
            i -= 1;
        }

        // i == 0 or get_key(i-1) < key
        get_key(i) = key;
        value->serialize(get_value(i));
    }

    // static void copy_cell(int cell_id) {

    // }
    virtual bool isFull() override { return num_cells() == num_max_cell; }

    // when current node is overloaded, split it into two
    // node of size (d+1) (d) then move pivot upward
    // pageid of the new page is known
    // key shall not duplicate
    /**
     * @brief
     *
     * @param key
     * @param value
     * @param new_page: address of new page which is not used by any node
     * @return uint32_t: pivot key whitch is the max key of left page
     */
    uint32_t insert_and_split(uint32_t key, Row * value, void * new_page)
    {
        // printf("enter\n");
        assert(isFull());
        // set statics of new_page
        LeafNode rightNode(new_page, row_size);

        // when leftNode is full -> left(full/2+1)  right(full/2)
        // find max i s.t. key < cell(i)
        // printf("start compare, num_cell=%d\n", num_cells());
        int n = num_cells();
        int key_pos = n;
        while (key_pos > 0 && get_key(key_pos - 1) > key)
            key_pos -= 1;
        // printf("key_pos=%d\n", key_pos);
        // key_pos == 0 or get_key(key_pos-1) <= key
        assert(key_pos == 0 && key < get_key(key_pos) || key_pos > 0 && get_key(key_pos - 1) < key);
        // key shall insert at key_pos
        //printf("End compare\n");

        // allocate memory in right node
        int left_load = n / 2 + 1, right_load = n / 2;
        for (int i = 0; i < right_load; ++i)
            rightNode.allocate_cell();

        // move old key & value pair into
        // left node and right node
        for (int i = n - 1; i >= 0; --i)
        {
            int idx = i + (key_pos <= i);

            // insert to left node with new id: idx
            if (idx < left_load)
            {
                // move from i to idx
                if (i != idx)
                    memcpy(get_cell(idx), get_cell(i), cell_size);

                // insert to right node
            }
            else
            {
                memcpy(rightNode.get_cell(idx - left_load), get_cell(i), cell_size);
            }
        }
        // printf("key inserted\n");

        // insert key_pos
        if (key_pos < left_load)
        {
            get_key(key_pos) = key;
            value->serialize(get_value(key_pos));
        }
        else
        {
            rightNode.get_key(key_pos - left_load) = key;
            value->serialize(rightNode.get_value(key_pos - left_load));
        }

        // addjust left node size to left_load
        num_cells() = left_load;

        return get_key(left_load - 1);
    }

    void initialize_leaf_node(void * node) { num_cells() = 0; }

    /*
    ROW_SIZE: 68
    COMMON_NODE_HEADER_SIZE: 10
    LEAF_NODE_HEADER_SIZE: 14
    LEAF_NODE_CELL_SIZE: 72
    LEAF_NODE_SPACE_FOR_CELLS: 4082
    LEAF_NODE_MAX_CELLS: 56
    */
    void print_constants()
    {
        printf("ROW_SIZE: %d\n", row_size);
        printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
        printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
        printf("LEAF_NODE_CELL_SIZE: %d\n", cell_size);
        printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
        printf("LEAF_NODE_MAX_CELLS: %d\n", num_max_cell);
    }

    void print_node()
    {
        printf("leaf (size %d)\n", num_cells());
        for (uint32_t i = 0; i < num_cells(); i++)
        {
            uint32_t key = get_key(i);
            printf("  - %d : %d\n", i, key);
        }
    }

    inline static uint32_t * extract_key(void * cell) { return (uint32_t *)cell; }

    inline static void * extract_value(void * cell) { return (char *)cell + LEAF_NODE_VALUE_OFFSET; }
};

/**
 * @brief layout of internal node
 * INTERNAL_NODE 4 byte
 * load: (pointer0, key0 ... pointer(n-1), key(n-1), pointer(n))
 * since Nodes shall persist into disk we use id of page to fill the pointers
 */
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint64_t);
struct InternalNode : public BtreeNode
{
    uint32_t num_max_keys; // init to even


    InternalNode(void * page, bool reset = false) : BtreeNode(page)
    {
        if (reset)
        {
            // set node type
            *((uint8_t *)((char *)data + NODE_TYPE_OFFSET)) = NODE_TYPE_INNER;
            num_keys() = 0;
        }

        // set num of max keys
        // since data loading is of form (p0,k0,p1,k1...,pn-1,kn-1,pn)
        uint32_t payload_size = PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE;
        uint32_t max_num_cell = (payload_size - INTERNAL_NODE_CHILD_SIZE) / (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE);
        num_max_keys = max_num_cell % 2 ? max_num_cell - 1 : max_num_cell;
    }

    virtual uint32_t get_node_load() const override { return num_max_keys; }

    virtual void set_node_load(uint32_t size) override
    {
        assert(size % 2 == 0 && size > 0);
        num_max_keys = size;
    }

    virtual uint32_t get_num_keys() const override
    {
        char * pos = (char *)data + INTERNAL_NODE_NUM_KEYS_OFFSET;
        return *((uint32_t *)pos);
    }

    uint32_t & num_keys()
    { // must init !!
        return *num_keys_ptr();
    }

    // get number of rows
    uint32_t * num_keys_ptr()
    {
        char * pos = (char *)data + INTERNAL_NODE_NUM_KEYS_OFFSET;
        return (uint32_t *)pos;
    }


    // get key of a cell
    // key is of form (p0,k0,....)
    virtual uint32_t & get_key(uint32_t id) override
    {
        assert(id < num_max_keys);

        char * start = (char *)data + INTERNAL_NODE_HEADER_SIZE;
        char * pos = start + id * (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE) + INTERNAL_NODE_CHILD_SIZE;

        return *((uint32_t *)pos);
    }

    // get value from a cell
    uint64_t & get_child(uint32_t id)
    {
        assert(id <= num_max_keys);
        char * start = (char *)data + INTERNAL_NODE_HEADER_SIZE;
        char * pos = start + id * (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE);

        return *((uint64_t *)pos);
    }

    // set value of a child
    // void set_child(uint32_t id, uint32_t page_id) {
    //     assert(id <= num_max_keys);
    //     char * start = (char *) data + INTERNAL_NODE_HEADER_SIZE;
    //     char * pos = start +
    //         id * (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE);

    //     *((int64_t *) pos) = page_id;
    // }


    /**
     * @brief insert (left_ptr, key, right_ptr) into current node
     *   assumption: current node is not full
     * @param key
     * @param left page id of left node, assume that all keys in left tree <= key
     * @param right page id of right node. assume all key in right tree > key
     */
    void insert(uint32_t key, uint64_t left, uint64_t right)
    {
        int n = num_keys();
        num_keys() += 1;

        // when current page is empty we shall insert key, left and right
        // into first position
        if (n == 0)
        {
            get_child(0) = left;
            get_key(0) = key;
            get_child(1) = right;
        }
        else
        {
            // printf("key=%d\n", key);
            int i = n - 1;
            while (i >= 0 && get_key(i) >= key)
            {
                assert(get_key(i) != key); // key not duplicate

                // move (key_i, child_{i+1}) to right position
                get_key(i + 1) = get_key(i);
                get_child(i + 2) = get_child(i + 1);
                i -= 1;
            }

            // when i == -1 => key < get_key(0)
            // insert (left, key, right) to (c0, k0, c1, k1)
            // -> (c0, key, right, k0, c1, k1)
            // i == -1 or get_key(i) < key
            get_key(i + 1) = key;
            get_child(i + 2) = right;

            assert(get_child(i + 1) == left);
        }
    }

    /**
     * @brief when internal node is full. insert (left, key, right) will overflow the page
     *  into p1' (size: n/2) (p1', key', p2') p2' (size: n/2)
     *  (p1', key', p2') will insert to upper level
     *  p1' is the id of current page and p2' is the id of newly allocated page
     * @param key
     * @param left page id of the left child
     * @param right page id of the right child
     * @param new_page
     * @return uint32_t
     */
    uint32_t insert_and_split(uint32_t key, uint64_t left, uint64_t right, void * new_page)
    {
        // interprete new page as a new inner node
        InternalNode rightNode(new_page, true);

        // allocate n/2 new entries in right node
        assert(isFull());
        int n = num_keys();
        rightNode.num_keys() = n / 2;

        // locate the position to insert current key
        int key_pos;
        bool success = search_insert_location(key, key_pos);
        assert(success);
        // printf("key_pos = %d\n", key_pos);

        // move old key & its right child to right position
        // node that pivot is put to idx == n / 2 of current node
        // the pivot's right child shall at be the first child
        // of the right node
        // for each move only (key, rightchild) is moved
        int left_load = n / 2 + 1, right_load = n / 2;
        int pivot_pos = n / 2; // at left_node
        for (int i = n - 1; i >= 0; --i)
        {
            // if i < key_pos it shall locate at i
            // if i >= key_pos its location becomes i+1
            // since key_pos shall occupied by the new key
            int idx = i + (key_pos <= i);

            // insert to left node with new id: idx
            // when idx == pivot_pos, its rchild shall
            // move to the start pos of right child
            if (idx == pivot_pos)
            {
                // move right child
                rightNode.get_child(0) = get_child(i + 1);

                // when idx == i+1 one shall move its key
                if (idx != i)
                    get_key(idx) = get_key(i);
            }
            else if (idx < left_load)
            {
                // idx == i + 1, move key and rightchild
                if (i != idx)
                {
                    get_key(idx) = get_key(i);
                    get_child(idx + 1) = get_child(i + 1);
                }

                // insert to right node
            }
            else
            {
                rightNode.get_key(idx - left_load) = get_key(i);
                rightNode.get_child(idx - left_load + 1) = get_child(i + 1);
            }
        }

        // insert new key to key_pos
        // special case: key_pos is pivot
        if (key_pos < left_load)
        {
            get_key(key_pos) = key;

            if (key_pos == pivot_pos)
                rightNode.get_child(0) = right;
            else
                get_child(key_pos + 1) = right;

            // assert left
            assert(get_child(key_pos) == left);
        }
        else
        {
            rightNode.get_key(key_pos - left_load) = key;
            rightNode.get_child(key_pos - left_load + 1) = right;
            assert(rightNode.get_child(key_pos - left_load) == left);
        }

        // remove tail elements of current node
        uint32_t pivot_key = get_key(pivot_pos);
        num_keys() = n / 2;

        // return the pivot of these keys which is
        // of rank n / 2
        return pivot_key;
    }

    /**
     * @brief search position before which the key shall insert
     * (not test yet)
     * @param key
     * @param pos
     * @return true: position found
     * @return false: search failed, duplicate found. in this
     *  circumstoms pos is set to the index of key
     */
    bool search_insert_location(uint32_t key, int & pos)
    {
        // int start = 0, end = num_keys() - 1;
        // while (start <= end)
        // {
        //     int mid = start + (end - start) / 2;
        //     uint32_t key_mid = get_key(mid);

        //     if (key_mid == key)
        //     {
        //         pos = mid;
        //         // duplicated key
        //         return false;

        //         // key[start .. mid] < key
        //     }
        //     else if (key_mid < key)
        //     {
        //         start = mid + 1;

        //         // key < key_mid
        //     }
        //     else
        //     {
        //         end = mid - 1;
        //     }
        // }

        // // success start > end
        // // key shall inserted as follow:
        // // end _   start
        // // _   key _
        // assert(start == end + 1);
        // pos = start;
        pos = search_key_position(key);
        if (pos >= 0 && get_key(pos) == key)
            return false;

        pos += 1;
        return true;
    }

    virtual bool isFull() override { return num_keys() == num_max_keys; }

    // max_num_keys: 338
    void print_node()
    {
        printf("inner node (size %d)\n", num_keys());
        for (uint32_t i = 0; i < num_keys(); i++)
        {
            uint64_t child = get_child(i);
            uint32_t key = get_key(i);
            printf("%lld <= %d < ", child, key);
        }
        printf("%lld\n", get_child(num_keys()));
    }
};

struct KeyLocation
{
    // page id which should contains the key
    uint64_t page_id;

    // if key exist row id is the cell id of the key
    // else row_id is the place to insert the key
    int row_id;

    // is key exist on current tree
    bool is_exist;

    // KeyLocation(uint64_t page_id) : page_id(page_id), row_id() { }
    KeyLocation(uint64_t page_id, uint32_t row_id, bool is_exist) : page_id(page_id), row_id(row_id), is_exist(is_exist) { }
};

// class for test
struct NaryTree;
/**
 * @brief B+Tree
 * protocal: for each inner node max(child_i) <= key_i < min(child_(i+1))
 * Pager: sync from memory and disk, set and get root id
 * leaf_load and inner_node_load
 * insert()
 */
class BPlusTree
{
public:
    BPlusTree(const std::string & path, char mode, uint32_t row_size, uint32_t leaf_node_load = 10, uint32_t inner_node_load = 10);

    uint64_t get_root_page() const;
    uint64_t get_total_page() const;
    // void visit_rows(std::function<Row*()> handler);
    // const Row* find(uint32_t key) const;
    enum class InsertStatus;
    InsertStatus insert(uint32_t, Row * rows);
    KeyLocation find(uint32_t key);
    void print_keys();


    // public properties
public:
    const uint32_t row_size;

    // public embeded structures
public:
    enum class InsertStatus
    {
        SUCCESS,
        FAIL_DUPLICATE_KEY
    };

private:
    void update_root(uint64_t page_id);
    void link_to(uint64_t child, uint64_t parent);
    std::unique_ptr<BtreeNode> get_node_by(uint64_t page_id) { return BtreeNode::LoadNodeFrom(pager.get_page(page_id)); }
    void post_order_visit(
        uint64_t page_id,
        std::function<void(uint64_t)> inner_node_action,
        std::function<void(void *)> leaf_cell_action,
        uint32_t min_key,
        uint32_t max_key);
    // check valid

    BTreePager pager;
    uint32_t leaf_load;
    uint32_t inner_node_load;
    void * root_page;
    std::unique_ptr<BtreeNode> root;

    friend struct NaryTree;
};