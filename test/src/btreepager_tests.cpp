#include "core/parameters.h"
#include <core/dbfile.h>
#include <gtest/gtest.h>
#include <sys/fcntl.h>
#include <unistd.h>

TEST(btreepager, metadata) {
    std::string path = "/tmp/metadata";
    int fd = open(
        path.c_str(),
        O_WRONLY | O_CREAT | O_TRUNC,
        S_IWUSR | S_IRUSR
    );

    BtreeMetaData meta(path, fd);
    meta.root_pid = 30;
    meta.num_pages = 96;
    meta.write_to_disk();
    close(fd);

    fd = open(path.c_str(), O_RDONLY, S_IWUSR | S_IRUSR);
    BtreeMetaData meta2(path, fd);
    meta2.load_from_disk();

    EXPECT_EQ(meta.num_pages, meta2.num_pages);
    EXPECT_EQ(meta.root_pid, meta2.root_pid);

    close(fd);
}

TEST(btreepager, allocate) {
    BTreePager *pager = new BTreePager("/tmp/allocate", 'c');
    EXPECT_EQ(pager->num_pages(), 0);

    void * page1, *page2, *page3;
    pager->allocate_page(page1);
    pager->allocate_page(page2);
    pager->allocate_page(page3);
    EXPECT_EQ(pager->num_pages(), 3);
    delete pager;

    BTreePager *pager2 = new BTreePager("/tmp/allocate", 'o');
    EXPECT_EQ(pager2->num_pages(), 3);
    delete pager2;
}

TEST(btreepager, write_page) {
    BTreePager *pager = new BTreePager("/tmp/write_page", 'c');

    void * page1, *page2, *page3;
    pager->allocate_page(page1);
    pager->allocate_page(page2);
    pager->allocate_page(page3);
    for (int i=0; i < PAGE_SIZE; ++i) {
        ((char *)page1)[i] = 'a';
        ((char *)page2)[i] = 'b';
        ((char *)page3)[i] = 'c';
    }

    delete pager;

    BTreePager *pager2 = new BTreePager("/tmp/write_page", 'o');
    char * p4, *p5, *p6;
    p5 = (char *) pager2->get_page(1);
    p6 = (char *) pager2->get_page(2);
    p4 = (char *) pager2->get_page(0);

    for (int i=0; i < PAGE_SIZE; ++i) {
        EXPECT_EQ(p4[i], 'a');
        EXPECT_EQ(p5[i], 'b');
        EXPECT_EQ(p6[i], 'c');
    }
    delete pager2;
}