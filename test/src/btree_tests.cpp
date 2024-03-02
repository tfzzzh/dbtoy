#include "core/parameters.h"
#include <_types/_uint32_t.h>
#include <gtest/gtest.h>
#include <core/btree.h>
#include <core/row.h>
#include <malloc/_malloc.h>
#include <string>
#include <vector>

TEST(btree, use_leaf_node) {
    UserInfo user(0, "alice", "alice@google.com");
    void * page = malloc(PAGE_SIZE);
    LeafNode lnode(page, user.get_row_byte());

    for (int i=0; i < lnode.num_max_cell; ++i) {
        void * cell = lnode.allocate_cell();
        *LeafNode::extract_key(cell) = i;
        user.serialize(LeafNode::extract_value(cell));
        // lnode.num_cells() += 1;
    }

    EXPECT_EQ(lnode.num_cells(), lnode.num_max_cell);

    UserInfo user_read;
    for (int i=0; i < lnode.num_cells(); ++i) {
        user_read.deserialize(lnode.get_value(i));
        EXPECT_EQ(user.to_string(), user_read.to_string());
        EXPECT_EQ(lnode.get_key(i), i);
    }

    free(page);
}

TEST(btree, check_duplicate) {
    int n = 20;
    void * page = malloc(PAGE_SIZE);
    LeafNode lnode(page, UserInfo().get_row_byte());
    for (int i=0; i < n; ++i) {
        UserInfo user(i);
        void * cell = lnode.allocate_cell();
        *LeafNode::extract_key(cell) = i;
        user.serialize(LeafNode::extract_value(cell));
    }

    for(int i=0; i < n; ++i)
        EXPECT_TRUE(lnode.is_duplicate(i));
    EXPECT_FALSE(lnode.is_duplicate(n));

    free(page);
}

TEST(btree, insert_leaf_node) {
    void * page = malloc(PAGE_SIZE);
    LeafNode lnode(page, UserInfo().get_row_byte());
    std::vector<Row*> rows({
        new UserInfo(5,"5","5"),
        new UserInfo(3,"3","3"),
        new UserInfo(2,"2","2"),
        new UserInfo(4,"4","4"),
        new UserInfo(0,"0","0"),
        new UserInfo(1,"1","1"),
    });

    lnode.insert(5, rows[0]);
    lnode.insert(3, rows[1]);
    lnode.insert(2, rows[2]);
    lnode.insert(4, rows[3]);
    lnode.insert(0, rows[4]);
    lnode.insert(1, rows[5]);

    EXPECT_EQ(lnode.num_cells(), 6);
    UserInfo user;
    for (int i=0; i < lnode.num_cells(); ++i) {
        EXPECT_EQ(lnode.get_key(i), i);
        user.deserialize(lnode.get_value(i));
        EXPECT_EQ(
            user.to_string(),
            std::to_string(i) + ',' + std::to_string(i) + ',' + std::to_string(i)
        );
    }

    for (int i=0; i < rows.size(); ++i) {
        delete rows[i];
    }

    free(page);
}

TEST(btree, insert_full_leaf_node) {
    void * page = malloc(PAGE_SIZE);
    void * rpage = malloc(PAGE_SIZE);
    LeafNode snode(page, UserInfo().get_row_byte());

    std::vector<uint32_t> skips = {0, 10,
        snode.num_max_cell-10, snode.num_max_cell-1};

    for(auto skip : skips) {
        printf("test %d\n",skip);
        LeafNode lnode(page, UserInfo().get_row_byte());
        for (int i=0; i <= lnode.num_max_cell; ++i) {
            if (i != skip) {
                UserInfo row(i,
                    std::to_string(i).c_str(),
                    std::to_string(i).c_str()
                );

                lnode.insert(i, &row);
            }
        }

        // allocate a new page and insert the skip
        UserInfo last(
            skip, std::to_string(skip).c_str(),
            std::to_string(skip).c_str()
        );
        uint32_t pivot = lnode.insert_and_split(skip, &last, rpage);
        EXPECT_EQ(pivot, lnode.num_max_cell / 2);

        for (int i=0; i < lnode.num_cells(); ++i) {
            UserInfo ui;
            ui.deserialize(lnode.get_value(i));
            EXPECT_TRUE(lnode.get_key(i) == i);
            EXPECT_TRUE(
                std::to_string(i) + "," +std::to_string(i) + "," +
                std::to_string(i)
                == ui.to_string()
            );
        }

        LeafNode rnode(rpage);
        EXPECT_EQ(rnode.num_cells() + 1, lnode.num_cells());
        for (int i=0; i < rnode.num_cells(); ++i) {
            int idx = i + lnode.num_cells();
            UserInfo ui;
            ui.deserialize(rnode.get_value(i));
            EXPECT_TRUE(rnode.get_key(i) == idx);
            EXPECT_TRUE(
                std::to_string(idx) + "," +std::to_string(idx) + "," +
                std::to_string(idx)
                == ui.to_string()
            );
        }
    }
    free(rpage);
    free(page);
}


TEST(btree, insert_inner_node) {
    // suppose the key ptr sequence is of form
    // (0',0,1',1,2',2,3',4,4',5,5')
    // key seq: (2, 5, 1, 3, 4, 0)
    // insert 2: (p0, 2, p1)
    // insert 5: (p1, 5, p2) -> (p0, 2, p1, 5, p2)
    // insert 1: (p0, 1, p3) -> (p0, 1, p3, 2, p1, 5, p2)
    // insert 3: (p1, 3, p4) -> (p0, 1, p3, 2, p1, 3, p4, 5, p2)
    // insert 4: (p4, 4, p5) -> (p0, 1, p3, 2, p1, 3, p4, 4, p5, 5, p2)
    // insert 0: (p0, 0, p6) -> (p0, 0, p6, 1, p3, 2, p1, 3, p4, 4, p5, 5, p2)
    void * page = malloc(PAGE_SIZE);
    InternalNode node(page, true);
    node.insert(2, 0, 1);
    node.insert(5, 1, 2);
    node.insert(1, 0, 3);
    node.insert(3, 1, 4);
    node.insert(4, 4, 5);
    node.insert(0, 0, 6);

    EXPECT_EQ(node.num_keys(), 6);
    int correct_pages[7] = {0,6,3,1,4,5,2};
    for (int i=0; i < 6; ++i) {
        EXPECT_EQ(node.get_key(i), i);
        EXPECT_EQ(node.get_child(i), correct_pages[i]);
    }
    EXPECT_EQ(node.get_child(6), correct_pages[6]);
    free(page);
}

TEST(btree, search_insert_location_fail) {
    void * page = malloc(PAGE_SIZE);

    InternalNode node(page, true);
    node.num_keys() = node.num_max_keys;
    for (int i=0; i < node.num_keys(); ++i) {
        node.get_key(i) = i;
    }

    for (int i=0; i < node.num_max_keys; ++i) {
        int pos;
        bool success = node.search_insert_location(i, pos);
        EXPECT_FALSE(success);
        EXPECT_EQ(pos, i);
    }

    free(page);
}

TEST(btree, insert_and_split) {
    void * page = malloc(PAGE_SIZE);

    // build a full node with key = 3k+1
    // lchild k, rchild k+1
    InternalNode lnode(page, true);
    lnode.num_keys() = lnode.num_max_keys;
    for (int i=0; i < lnode.num_max_keys; ++i) {
        lnode.get_child(i) = i;
        lnode.get_key(i) = 3 * i + 1;
    }
    lnode.get_child(lnode.num_max_keys) = lnode.num_max_keys;

    // insert 3*k to 0-th ... num_max_keys position
    void * page_backup = malloc(PAGE_SIZE);
    void * new_page = malloc(PAGE_SIZE);
    memcpy(page_backup, page, PAGE_SIZE);
    for (int i=0; i <= lnode.num_max_keys; ++i) {
        uint32_t new_key = 3 * i;
        uint64_t left = lnode.get_child(i);
        uint64_t right = lnode.num_max_keys + 1;

        uint32_t pivot = lnode.insert_and_split(new_key, left, right, new_page);

        // now the keys becomes (1, 4 ... 3*i-1 .. 3*i ... 3*i+1)
        if (i == lnode.num_max_keys / 2)
            EXPECT_EQ(new_key, pivot);
        else if (i < lnode.num_max_keys / 2)
            EXPECT_EQ(3*(lnode.num_max_keys/2-1)+1, pivot);
        else
            EXPECT_EQ(3*lnode.num_max_keys/2+1, pivot);

        // check keys
        InternalNode rNode(new_page, false);
        EXPECT_EQ(lnode.num_keys(), lnode.num_max_keys/2);
        EXPECT_EQ(rNode.num_keys(), lnode.num_max_keys/2);
        for (int j=0; j < lnode.num_keys(); ++j) {
            if (j < i) {
                EXPECT_EQ(lnode.get_key(j), 3*j+1);
            } else if (j > i) {
                EXPECT_EQ(lnode.get_key(j), 3*(j-1)+1);
            }
        }

        for (int j=0; j < rNode.num_keys(); ++j) {
            int idx = j + lnode.num_max_keys / 2 + 1;
            if (idx < i) {
                EXPECT_EQ(rNode.get_key(j), 3*idx+1);
            } else if(idx > i) {
                EXPECT_EQ(rNode.get_key(j), 3*(idx-1)+1);
            }
        }

        memcpy(lnode.data, page_backup, PAGE_SIZE);
        lnode.num_keys() = lnode.num_max_keys;
        // break;
    }

    free(new_page);
    free(page_backup);
    free(page);
}