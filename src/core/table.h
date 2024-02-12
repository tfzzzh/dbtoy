#pragma once
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <string>
#include "parameters.h"
#include "dbfile.h"

// const size_t PAGE_SIZE = 4096;
// const size_t TABLE_MAX_PAGES = 100;

/**
 * @brief memory to store rows of a table
 *
 */
class Table {
public:
    Table(size_t rsize, const std::string & file_path);
    ~Table();

    size_t get_row_per_page() const {
        assert(row_size > 0);
        return PAGE_SIZE / row_size;
    }

    size_t get_max_rows() const {
        return TABLE_MAX_PAGES * get_row_per_page();
    }

    // size_t get_num_rows() const {
    //     return num_rows;
    // }
    size_t num_rows;

    /**
     * @brief Get the row slot object
     *
     * @param rowid id of the row
     * @return void* position of the row
     */
    void * get_row_slot(size_t rowid);

private:
    // void * pages[TABLE_MAX_PAGES];
    // size_t num_rows;
    DbFile * dbfile;
    size_t row_size;
};

class TableBuffer {
public:
    static Table & get_instance() {
        static Table table(TableBuffer::row_size, path);
        return table;
    }

    static size_t row_size;
    static std::string path;

private:
    TableBuffer() = delete;
    TableBuffer(const TableBuffer & rhs) = delete;
};