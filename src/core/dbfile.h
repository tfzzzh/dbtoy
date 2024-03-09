#pragma once
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <_types/_uint64_t.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "parameters.h"

class Pager
{
public:
    virtual ~Pager(){};
    virtual void * get_page(int page_id) = 0;
    virtual void flush_page(int page_id, size_t size = PAGE_SIZE) = 0;
};

class DbFile : public Pager
{
public:
    DbFile(const std::string & path);
    virtual ~DbFile() override;
    virtual void * get_page(int page_id) override;
    virtual void flush_page(int page_id, size_t size = PAGE_SIZE) override;

    const std::string file_path;
    int file_descriptor;
    uint32_t length;
    void * pages[TABLE_MAX_PAGES];
    // num_pages in original file
    uint32_t num_pages;

private:
    off_t get_file_length();
};


struct BtreeMetaData
{
    const std::string file_path;
    const int file_descriptor;
    uint64_t root_pid;
    uint64_t num_pages;

    BtreeMetaData(const std::string & path, int fd) : file_path(path), file_descriptor(fd)
    {
        root_pid = 0;
        num_pages = 0;
    }

    int inline get_data_size() const { return sizeof(uint64_t) + sizeof(uint64_t); }

    // write meta data into file
    void write_to_disk();

    // load meta data from disk
    void load_from_disk();
};

class BTreePager : public Pager
{
public:
    // mode: 'c' : create new database
    // mode: 'o' : open exist database for io
    BTreePager(const std::string & path, char mode);
    virtual ~BTreePager() override;
    virtual void * get_page(int page_id) override;
    virtual void flush_page(int page_id, size_t size = PAGE_SIZE) override;
    uint64_t allocate_page(void *& new_page);
    inline uint64_t num_pages() const { return metaData->num_pages; }

    inline uint64_t get_root_page() const { return metaData->root_pid; }

    inline void set_root_page(uint64_t page_id) { metaData->root_pid = page_id; }

private:
    void sync(int page_id);

private:
    // shall remove at close
    BtreeMetaData * metaData;
    void * pages[TABLE_MAX_PAGES];
};

/**
 * @brief fill a buffer from file until buffer is full
 *          or EOF reached
 *
 * @param fd: file descriptor
 * @param dst: start position of the buffer
 * @param buffer_size
 * @return ssize_t: true bytes loaded
 */
ssize_t read_buffer(int fd, void * dst, size_t buffer_size);

/**
 * @brief write a buffer of size buffer_size into disk
 *
 * @param fd: file descriptor
 * @param src: start position of the buffer
 * @param buffer_size
 * @return ssize_t: true bytes loaded
 */
ssize_t write_buffer(int fd, void * src, size_t buffer_size);