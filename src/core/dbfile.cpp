#include "dbfile.h"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "parameters.h"

DbFile::DbFile(const std::string & path) : file_path(path)
{
    // open file descriptor
    file_descriptor = open(path.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

    if (file_descriptor < 0)
    {
        perror("open file error");
        exit(EXIT_FAILURE);
    }

    length = get_file_length();

    num_pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;

    // when init all page is not loaded
    for (int i = 0; i < TABLE_MAX_PAGES; ++i)
        pages[i] = nullptr;
}

off_t DbFile::get_file_length()
{
    struct stat buf;
    if (fstat(file_descriptor, &buf) < 0)
    {
        std::string msg = "get file state error, fd=" + std::to_string(file_descriptor);
        perror(msg.c_str());
        exit(EXIT_FAILURE);
    }
    return buf.st_size;
}

void * DbFile::get_page(int page_id)
{
    // error
    if (page_id >= TABLE_MAX_PAGES)
    {
        printf("page_id %d greater than max pages %ld\n", page_id, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
        return nullptr;
    }

    // when page is already in memory
    if (pages[page_id] != nullptr)
        return pages[page_id];

    // malloc a page: shall remove when flush
    void * page = malloc(PAGE_SIZE);
    size_t payload_bytes = 0;

    // fill contents in page
    if (page_id < num_pages)
    {
        off_t page_start = page_id * PAGE_SIZE;
        off_t status = lseek(file_descriptor, page_start, SEEK_SET);
        if (status < 0)
        {
            perror("lseek error");
            exit(EXIT_FAILURE);
        }
        assert(status == page_start);

        // read from page_start to the end
        auto nbytes = read_buffer(file_descriptor, page, PAGE_SIZE);

        assert(nbytes >= 0);
        payload_bytes = nbytes;
    }

    pages[page_id] = page;
    return page;
}

void DbFile::flush_page(int page_id, size_t size)
{
    if (pages[page_id] == nullptr)
        return;

    // seek to write position
    off_t page_start = page_id * PAGE_SIZE;
    off_t status = lseek(file_descriptor, page_start, SEEK_SET);
    if (status < 0)
    {
        perror("lseek error");
        exit(EXIT_FAILURE);
    }
    assert(status == page_start);

    // write data to disk
    auto nbytes = write_buffer(file_descriptor, pages[page_id], size);
    assert(nbytes >= 0);

    // free memeory
    free(pages[page_id]);
    pages[page_id] = nullptr;
}


DbFile::~DbFile()
{
    // close file
    close(file_descriptor);
    // delete page
    // flush data into disk
}

ssize_t read_buffer(int fd, void * dst, size_t buffer_size)
{
    ssize_t nread = 0;
    while (buffer_size > 0)
    {
        ssize_t nbyte = read(fd, dst, buffer_size);
        if (nbyte == 0)
            return nread;

        // error meet
        if (nbyte < 0)
        {
            perror("read buffer error");
            exit(EXIT_FAILURE);
            return nbyte;
        }

        nread += nbyte;
        buffer_size -= nbyte;
        dst = (char *)dst + nbyte;
    }

    return nread;
}

ssize_t write_buffer(int fd, void * src, size_t buffer_size)
{
    ssize_t nwrite = 0;

    while (buffer_size > 0)
    {
        ssize_t nbyte = write(fd, src, buffer_size);

        if (nbyte == -1)
        {
            perror("write buffer error");
            exit(EXIT_FAILURE);
            return -1;
        }

        nwrite += nbyte;
        buffer_size -= nbyte;
        src = (char *)src + nbyte;
    }

    return nwrite;
}

void BtreeMetaData::write_to_disk()
{
    // meta data is located at the front page
    off_t status = lseek(file_descriptor, 0, SEEK_SET);
    assert(status == 0);

    write_buffer(file_descriptor, &root_pid, sizeof(uint64_t));
    write_buffer(file_descriptor, &num_pages, sizeof(uint64_t));
}

void BtreeMetaData::load_from_disk()
{
    // meta data is located at the front page
    off_t status = lseek(file_descriptor, 0, SEEK_SET);
    assert(status == 0);

    read_buffer(file_descriptor, &root_pid, sizeof(uint64_t));
    read_buffer(file_descriptor, &num_pages, sizeof(uint64_t));
}


BTreePager::BTreePager(const std::string & path, char mode)
{
    int fd = -1;
    if (mode == 'c')
    {
        fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    }
    else if (mode == 'o')
    {
        fd = open(path.c_str(), O_RDWR, S_IWUSR | S_IRUSR);
    }
    else
    {
        fprintf(stderr, "mode shall be 'o' or 'c'");
        exit(EXIT_FAILURE);
    }

    if (fd < 0)
    {
        perror("open file error");
        exit(EXIT_FAILURE);
    }

    // load meta data
    metaData = new BtreeMetaData(path, fd);
    if (mode == 'o')
        metaData->load_from_disk();

    for (int i = 0; i < TABLE_MAX_PAGES; ++i)
        pages[i] = nullptr;

    // std::cout << "here out of constructor" << std::endl;
}

BTreePager::~BTreePager()
{
    // flush?
    metaData->write_to_disk();
    for (int i = 0; i < metaData->num_pages; ++i)
        flush_page(i);

    close(metaData->file_descriptor);
    delete metaData;
}

void * BTreePager::get_page(int page_id)
{
    // error: page_id > num_page
    auto num_pages = metaData->num_pages;
    if (page_id >= num_pages)
    {
        fprintf(stderr, "page_id %d greater than capacity %lld\n", page_id, num_pages);
        exit(EXIT_FAILURE);
        return nullptr;
    }

    // when page is already in memory
    if (pages[page_id] != nullptr)
        return pages[page_id];

    // malloc a page: shall remove when flush
    void * page = malloc(PAGE_SIZE);

    // load data from disk
    off_t page_start = metaData->get_data_size() + page_id * PAGE_SIZE;
    off_t status = lseek(metaData->file_descriptor, page_start, SEEK_SET);
    if (status < 0)
    {
        fprintf(stderr, "lseek error\n");
        exit(EXIT_FAILURE);
    }
    assert(status == page_start);
    auto nbytes = read_buffer(metaData->file_descriptor, page, PAGE_SIZE);
    assert(nbytes == PAGE_SIZE);

    // push page into cache
    pages[page_id] = page;

    return page;
}

uint64_t BTreePager::allocate_page(void *& new_page)
{
    // allocate memory
    void * page = malloc(PAGE_SIZE);

    // change meta data
    metaData->num_pages += 1;
    pages[metaData->num_pages - 1] = page;

    // write back meta data
    metaData->write_to_disk();
    // sync to disk?

    new_page = page;
    return metaData->num_pages - 1;
}

void BTreePager::sync(int page_id)
{
    if (pages[page_id] == nullptr)
        return;

    // seek to write position
    off_t page_start = metaData->get_data_size() + page_id * PAGE_SIZE;
    off_t status = lseek(metaData->file_descriptor, page_start, SEEK_SET);
    if (status < 0)
    {
        fprintf(stderr, "lseek error\n");
        exit(EXIT_FAILURE);
    }
    assert(status == page_start);

    // write page back to disk
    auto nbytes = write_buffer(metaData->file_descriptor, pages[page_id], PAGE_SIZE);

    assert(nbytes == PAGE_SIZE);
}

void BTreePager::flush_page(int page_id, size_t size)
{
    if (pages[page_id] == nullptr)
        return;
    sync(page_id);
    free(pages[page_id]);
    pages[page_id] = nullptr;
}