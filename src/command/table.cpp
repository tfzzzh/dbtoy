#include <cstddef>
#include <cstdlib>
#include <table.h>

Table::Table(size_t rsize): row_size(rsize) {
    num_rows = 0;
    for (size_t i=0; i < TABLE_MAX_PAGES; ++i) {
        pages[i] = nullptr;
    }
}

Table::~Table() {
    for (size_t i=0; i < TABLE_MAX_PAGES; ++i) {
        if (pages[i] != nullptr) {
            free(pages[i]);
            pages[i] = nullptr;
        }
    }
    num_rows = 0;
}

void * Table::get_row_slot(size_t rowid) {
    // get the page id of the row
    size_t page_id = rowid / get_row_per_page();
    assert(page_id < PAGE_SIZE);

    size_t offset = (rowid % get_row_per_page()) * row_size;

    if (pages[page_id] == nullptr)
        pages[page_id] = malloc(PAGE_SIZE);

    void * pos = ((char *) pages[page_id]) + offset;
    return pos;
}