#include "parameters.h"
#include "row.h"
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <sys/_types/_int64_t.h>

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
const uint32_t PARENT_POINTER_SIZE = sizeof(int64_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;
const int64_t NODE_PARENT_INVALID = -1;

struct BtreeNode {
    void * data; // pointer to a page

    BtreeNode(void * page): data(page) {
        // the page shall known
        // data = malloc(PAGE_SIZE);

        // one shall not change *page when it is the only parameter of
        // constructor
        // defaultly the node is not a root node
        // set_root(false);
        // parent() = NODE_PARENT_INVALID;
    }

    const uint8_t & node_type() const {
        const uint8_t & value = *((uint8_t*)((char *)data + NODE_TYPE_OFFSET));
        return value;
    }

    bool is_root() const {
        bool result = *(
            (char *) data + IS_ROOT_OFFSET
        );
        return result;
    }

    void set_root(bool is_root=true) {
        *(
            (char *) data + IS_ROOT_OFFSET
        ) = (int) is_root;

    }

    // when current node's parent is not valid set it to -1
    int64_t & parent() {
        return *(int64_t *) ((char *) data + PARENT_POINTER_OFFSET);
    }

    // virtual ~BtreeNode() {
    //     free(data);
    // }
};

/**
 * @brief header of leaf node layout
 * NUM_CELLS 4 byte
 * row_size 4 byte
 */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_ROWSIZE_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_ROWSIZE_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET +
    LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_ROWSIZE_SIZE;

/**
 * @brief layout of payload (key, value)
 *
 */
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_OFFSET =
    LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;

const uint8_t NODE_TYPE_INNER = 0;
const uint8_t NODE_TYPE_LEAF = 1;


/**
 * @brief leafnode of b+tree
 *
 */
struct LeafNode : BtreeNode{
    uint32_t row_size; // size of a row
    uint32_t cell_size; // concate of key and row
    uint32_t num_max_cell; // shall never change them

    LeafNode(void * page, uint32_t row_size):
        BtreeNode(page),row_size(row_size){
        cell_size = sizeof(uint32_t) + row_size;
        num_max_cell =  LEAF_NODE_SPACE_FOR_CELLS / cell_size;

        // enforce that the load of tree is even
        if (num_max_cell % 2 == 1)
            num_max_cell -= 1;

        // set num_cell to 0
        num_cells() = 0;

        assert(num_max_cell > 0);

        // set node type
        *((uint8_t*)((char *)data + NODE_TYPE_OFFSET)) =
            NODE_TYPE_LEAF;

        // set row size
        *get_rowsize_ptr() = row_size;
    }

    // build node directly from page
    LeafNode(void * page): BtreeNode(page) {
        row_size = *get_rowsize_ptr();

        cell_size = sizeof(uint32_t) + row_size;
        num_max_cell =  LEAF_NODE_SPACE_FOR_CELLS / cell_size;

        // enforce that the load of tree is even
        if (num_max_cell % 2 == 1)
            num_max_cell -= 1;

        assert(num_max_cell > 0);
    }

    // get pointer to row_size
    uint32_t * get_rowsize_ptr() {
        return (uint32_t *) ((char *) data + LEAF_NODE_ROWSIZE_OFFSET);
    }

    // num of (key, value) pairs
    // note that :: add num_cells by one when new row inserted
    uint32_t & num_cells() {
        uint32_t * num_cells_ptr = (uint32_t *)
            ((char *) data + (size_t) LEAF_NODE_NUM_CELLS_OFFSET);
        return *num_cells_ptr;
    }

    // pair of (key, val)
    // cell_num < leaf_node_num_cells
    void* get_cell(uint32_t cell_num) {
        assert(cell_num < num_cells());
        return (char *) data + LEAF_NODE_HEADER_SIZE + cell_num * cell_size;
    }

    // allocate new cell from the page
    // when no slot available return nullptr
    void* allocate_cell() {
        uint32_t m = num_cells();
        if (m == num_max_cell) return nullptr;

        void * pos = (char *) data + LEAF_NODE_HEADER_SIZE + m * cell_size;

        num_cells() += 1;
        return pos;
    }

    // get key of a cell
    uint32_t & get_key(uint32_t cell_num) {
        return *((uint32_t*) get_cell(cell_num));
    }

    // pointer to value content, shall deserialize using row object
    void* get_value(uint32_t cell_num) {
        void * cell_ptr = get_cell(cell_num);
        return (char *) cell_ptr + LEAF_NODE_VALUE_OFFSET;
    }

    bool is_duplicate(uint32_t key) {
        for (uint32_t i=0; i < num_cells(); ++i) {
            if (get_key(i) == key)
                return true;
        }
        return false;
    }

    // duplicate is not checked using this method
    // insert (key, value) into the page make the page sorted by key
    void insert(uint32_t key, Row * value) {
        assert(num_cells() < num_max_cell);
        auto m = num_cells();

        // allocate a place for the newly insert row
        allocate_cell();
        auto i = m;
        while (i >= 1 && get_key(i-1) > key) {
            // copy cell[i-1] to cell[i]
            memcpy(
                get_cell(i),
                get_cell(i-1),
                cell_size
            );
            i -= 1;
        }

        // i == 0 or get_key(i-1) < key
        get_key(i) = key;
        value->serialize(get_value(i));
    }

    // static void copy_cell(int cell_id) {

    // }
    bool isFull() {
        return num_cells() == num_max_cell;
    }

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
    uint32_t insert_and_split(uint32_t key, Row * value, void * new_page) {
        // printf("enter\n");
        assert(isFull());
        // set statics of new_page
        LeafNode rightNode(new_page, row_size);

        // when leftNode is full -> left(full/2+1)  right(full/2)
        // find max i s.t. key < cell(i)
        // printf("start compare, num_cell=%d\n", num_cells());
        int n = num_cells();
        int key_pos = n;
        while (key_pos > 0 && get_key(key_pos-1) > key)
            key_pos -= 1;
        // printf("key_pos=%d\n", key_pos);
        // key_pos == 0 or get_key(key_pos-1) <= key
        assert(
            key_pos == 0 && key < get_key(key_pos) ||
            key_pos > 0 && get_key(key_pos-1) < key
        );
        // key shall insert at key_pos
        //printf("End compare\n");

        // allocate memory in right node
        int left_load = n / 2 + 1, right_load = n / 2;
        for (int i=0; i < right_load; ++i)
            rightNode.allocate_cell();

        // move old key & value pair into
        // left node and right node
        for (int i=n-1; i >=0; --i) {
            int idx = i + (key_pos <= i);

            // insert to left node with new id: idx
            if (idx < left_load) {
                // move from i to idx
                if (i != idx) {
                    memcpy(get_cell(idx), get_cell(i),
                     cell_size);
                }

            // insert to right node
            } else {
                memcpy(
                    rightNode.get_cell(idx-left_load),
                    get_cell(i),
                    cell_size
                );
            }
        }
        // printf("key inserted\n");

        // insert key_pos
        if (key_pos < left_load) {
            get_key(key_pos) = key_pos;
            value->serialize(get_value(key_pos));
        } else {
            rightNode.get_key(key_pos - left_load) = key_pos;
            value->serialize(rightNode.get_value(key_pos-left_load));
        }

        // addjust left node size to left_load
        num_cells() = left_load;

        return get_key(left_load - 1);
    }

    void initialize_leaf_node(void* node) { num_cells() = 0; }

    /*
    ROW_SIZE: 68
    COMMON_NODE_HEADER_SIZE: 10
    LEAF_NODE_HEADER_SIZE: 14
    LEAF_NODE_CELL_SIZE: 72
    LEAF_NODE_SPACE_FOR_CELLS: 4082
    LEAF_NODE_MAX_CELLS: 56
    */
    void print_constants() {
        printf("ROW_SIZE: %d\n", row_size);
        printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
        printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
        printf("LEAF_NODE_CELL_SIZE: %d\n", cell_size);
        printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
        printf("LEAF_NODE_MAX_CELLS: %d\n", num_max_cell);
    }

    void print_node() {
        printf("leaf (size %d)\n", num_cells());
        for (uint32_t i = 0; i < num_cells(); i++) {
            uint32_t key = get_key(i);
            printf("  - %d : %d\n", i, key);
        }
    }

    inline static uint32_t * extract_key(void * cell){
        return (uint32_t*) cell;
    }

    inline static void * extract_value(void * cell){
        return (char *) cell + LEAF_NODE_VALUE_OFFSET;
    }
};

/**
 * @brief layout of internal node
 * INTERNAL_NODE 4 byte
 * load: (pointer0, key0 ... pointer(n-1), key(n-1), pointer(n))
 * since Nodes shall persist into disk we use id of page to fill the pointers
 */
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                        INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(int64_t);
struct InternalNode : BtreeNode {
    uint32_t num_max_keys; // init to even


    InternalNode(void * page): BtreeNode(page) {
        // set node type
        *((uint8_t*)((char *)data + NODE_TYPE_OFFSET)) =
            NODE_TYPE_INNER;

        // set num of max keys
        // since data loading is of form (p0,k0,p1,k1...,pn-1,kn-1,pn)
        uint32_t payload_size = PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE;
        uint32_t max_num_cell = (payload_size - INTERNAL_NODE_CHILD_SIZE) /
            (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE);
        num_max_keys = max_num_cell % 2 ? max_num_cell - 1 : max_num_cell;

        num_keys() = 0;
    }

    uint32_t &num_keys() { // must init !!
        return *num_keys_ptr();
    }

    // get number of rows
    uint32_t * num_keys_ptr() {
        char * pos = (char *) data + INTERNAL_NODE_NUM_KEYS_OFFSET;
        return (uint32_t *) pos;
    }


    // get key of a cell
    // key is of form (p0,k0,....)
    uint32_t & get_key(uint32_t id) {
        assert(id < num_max_keys);

        char * start = (char *) data + INTERNAL_NODE_HEADER_SIZE;
        char * pos = start +
            id * (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE) +
            INTERNAL_NODE_CHILD_SIZE;

        return * ((uint32_t *) pos);
    }

    // get value from a cell
    uint64_t & get_child(uint32_t id) {
        assert(id <= num_max_keys);
        char * start = (char *) data + INTERNAL_NODE_HEADER_SIZE;
        char * pos = start +
            id * (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE);

        return *((uint64_t *) pos);
    }

    // set value of a child
    // void set_child(uint32_t id, uint32_t page_id) {
    //     assert(id <= num_max_keys);
    //     char * start = (char *) data + INTERNAL_NODE_HEADER_SIZE;
    //     char * pos = start +
    //         id * (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE);

    //     *((int64_t *) pos) = page_id;
    // }


    void print_node() {
        printf("inner node (size %d)\n", num_keys());
        for (uint32_t i = 0; i < num_keys(); i++) {
            uint64_t child = get_child(i);
            uint32_t key = get_key(i);
            printf("%lld <= %d < ", child, key);
        }
        printf("%lld\n", get_child(num_keys()));
    }
};