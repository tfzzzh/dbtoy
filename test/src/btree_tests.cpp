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