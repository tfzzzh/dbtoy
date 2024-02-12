#pragma once
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "parameters.h"

class DbFile {
public:
    DbFile(const std::string & path);
    ~DbFile();
    void * get_page(int page_id);
    void flush_page(int page_id, size_t size=PAGE_SIZE);

    const std::string file_path;
    int file_descriptor;
    uint32_t length;
    void* pages[TABLE_MAX_PAGES];
    // num_pages in original file
    uint32_t num_pages;

private:
    off_t get_file_length();
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