#include "parameters.h"
#include <cstdlib>
#include <cstdio>
#include <cassert>

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
const uint32_t PARENT_POINTER_SIZE = sizeof(void *);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/**
 * @brief header of leaf node layout
 * NUM_CELLS 4 byte
 */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

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

struct BtreeNode {
    void * data; // pointer to a page

    BtreeNode() {
        data = malloc(PAGE_SIZE);
    }

    virtual ~BtreeNode() {
        free(data);
    }
};

struct LeafNode : BtreeNode{
    uint32_t row_size; // size of a row
    uint32_t cell_size; // concate of key and row
    uint32_t num_max_cell; //

    LeafNode(uint32_t row_size): BtreeNode(),row_size(row_size){
        cell_size = sizeof(uint32_t) + row_size;
        num_max_cell =  LEAF_NODE_SPACE_FOR_CELLS / cell_size;

        // set node type
        *((uint8_t*)((char *)data + NODE_TYPE_OFFSET)) =
            NODE_TYPE_LEAF;
    }

    // num of (key, value) pairs
    // note that :: add num_cells by one when new row inserted
    uint32_t & num_cells() {
        uint32_t * num_cells_ptr = (uint32_t *)
            ((char *) data + (size_t) LEAF_NODE_NUM_CELLS_OFFSET);
        return *num_cells_ptr;
    }

    const uint8_t & node_type() const {
        const uint8_t & value = *((uint8_t*)((char *)data + NODE_TYPE_OFFSET));
        return value;
    }

    // pair of (key, val)
    // cell_num < leaf_node_num_cells
    void* get_cell(uint32_t cell_num) {
        assert(cell_num < num_max_cell);
        return (char *) data + LEAF_NODE_HEADER_SIZE + cell_num * cell_size;
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
};