#include <cstddef>
#include <cstdlib>
#include <string>
#include "parameters.h"
#include "table.h"

Table::Table(size_t rsize, const std::string & file_path): row_size(rsize) {
    num_rows = 0;
    dbfile = new DbFile(file_path);

    size_t file_length = dbfile->length;
    size_t num_page = dbfile->num_pages;
    size_t remain_byte = file_length % PAGE_SIZE;

    if (remain_byte == 0) {
        num_rows = num_page * get_row_per_page();
    } else {
        assert(remain_byte % row_size == 0);
         num_rows = (num_page-1) * get_row_per_page() +
            remain_byte / row_size;
    }
}

Table::~Table() {
    // flush pages
    size_t row_per_page = get_row_per_page();
    size_t total_pages = (num_rows + row_per_page - 1) / row_per_page;

    // page 0 .. total_pages-2 shall flush with full size
    for (size_t i=0; i + 1 < total_pages; ++i) {
        dbfile->flush_page(i);
    }

    // flush remain part
    size_t remain_row = num_rows % row_per_page;
    if (remain_row == 0) {
        dbfile->flush_page(total_pages-1);
    } else {
        dbfile->flush_page(total_pages-1,
            remain_row * row_size);
    }

    num_rows = 0;

    delete dbfile;
}

void * Table::get_row_slot(size_t rowid) {
    // get the page id of the row
    // size_t page_id = rowid / get_row_per_page();
    // assert(page_id < PAGE_SIZE);

    // size_t offset = (rowid % get_row_per_page()) * row_size;

    // if (pages[page_id] == nullptr)
    //     pages[page_id] = malloc(PAGE_SIZE);

    // void * pos = ((char *) pages[page_id]) + offset;
    // return pos;

    // rowid = page_id * row_per_page + offset
    size_t page_id = rowid / get_row_per_page();
    size_t offset = rowid % get_row_per_page();

    // get page from buffer
    void * page_base = dbfile->get_page(page_id);

    void * pos = (char *) page_base + offset * row_size;

    return pos;
}

// static variable
size_t TableBuffer::row_size = 0;
std::string TableBuffer::path = "";