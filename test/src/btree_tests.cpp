#include <gtest/gtest.h>
#include <core/btree.h>
#include <core/row.h>

TEST(btree, use_leaf_node) {
    UserInfo user(0, "alice", "alice@google.com");
    LeafNode lnode(user.get_row_byte());

    for (int i=0; i < lnode.num_max_cell; ++i) {
        lnode.get_key(i) = i;
        user.serialize(lnode.get_value(i));
        lnode.num_cells() += 1;
    }

    EXPECT_EQ(lnode.num_cells(), lnode.num_max_cell);

    UserInfo user_read;
    for (int i=0; i < lnode.num_cells(); ++i) {
        user_read.deserialize(lnode.get_value(i));
        EXPECT_EQ(user.to_string(), user_read.to_string());
        EXPECT_EQ(lnode.get_key(i), i);
    }
}

TEST(btree, check_duplicate) {
    int n = 20;
    LeafNode lnode(UserInfo().get_row_byte());
    for (int i=0; i < n; ++i) {
        UserInfo user(i);
        lnode.get_key(i) = i;
        user.serialize(lnode.get_value(i));
        lnode.num_cells() += 1;
    }

    for(int i=0; i < n; ++i)
        EXPECT_TRUE(lnode.is_duplicate(i));
    EXPECT_FALSE(lnode.is_duplicate(n));
}