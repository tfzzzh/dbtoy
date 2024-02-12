#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "dbfile.h"
#include "parameters.h"

DbFile::DbFile(const std::string & path):
    file_path(path) {
    // open file descriptor
    file_descriptor = open(path.c_str(),
        O_RDWR | O_CREAT,
        S_IWUSR | S_IRUSR );

    if (file_descriptor < 0) {
        perror("open file error");
        exit(EXIT_FAILURE);
    }

    length = get_file_length();

    num_pages = (length + PAGE_SIZE - 1 ) / PAGE_SIZE;

    // when init all page is not loaded
    for (int i=0; i < TABLE_MAX_PAGES; ++i) {
        pages[i] = nullptr;
    }
}

off_t DbFile::get_file_length() {
    struct stat buf;
    if (fstat(file_descriptor, &buf) < 0) {
        std::string msg = "get file state error, fd=" +
            std::to_string(file_descriptor);
        perror(msg.c_str());
        exit(EXIT_FAILURE);
    }
    return buf.st_size;
}

void * DbFile::get_page(int page_id) {
    // error
    if (page_id >= TABLE_MAX_PAGES) {
        printf(
            "page_id %d greater than max pages %ld\n",
            page_id, TABLE_MAX_PAGES
        );
        exit(EXIT_FAILURE);
        return nullptr;
    }

    // when page is already in memory
    if (pages[page_id] != nullptr) {
        return pages[page_id];
    }

    // malloc a page: shall remove when flush
    void * page = malloc(PAGE_SIZE);
    size_t payload_bytes = 0;

    // fill contents in page
    if (page_id < num_pages) {
        off_t page_start = page_id * PAGE_SIZE;
        off_t status = lseek(file_descriptor, page_start, SEEK_SET);
        if (status < 0) {
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

void DbFile::flush_page(int page_id, size_t size) {
    if (pages[page_id] == nullptr) return;

    // seek to write position
    off_t page_start = page_id * PAGE_SIZE;
    off_t status = lseek(file_descriptor, page_start, SEEK_SET);
    if (status < 0) {
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


DbFile::~DbFile() {
    // close file
    close(file_descriptor);
    // delete page
    // flush data into disk
}

ssize_t read_buffer(int fd, void * dst, size_t buffer_size) {
    ssize_t nread = 0;
    while (buffer_size > 0) {
        ssize_t nbyte = read(fd, dst, buffer_size);
        if (nbyte == 0) {
            return nread;
        }

        // error meet
        if (nbyte < 0) {
            perror("read buffer error");
            exit(EXIT_FAILURE);
            return nbyte;
        }

        nread += nbyte;
        buffer_size -= nbyte;
        dst = (char *) dst + nbyte;
    }

    return nread;
}

ssize_t write_buffer(int fd, void * src, size_t buffer_size) {
    ssize_t nwrite = 0;

    while (buffer_size > 0) {
        ssize_t nbyte = write(fd, src, buffer_size);

        if (nbyte == -1) {
            perror("write buffer error");
            exit(EXIT_FAILURE);
            return -1;
        }

        nwrite += nbyte;
        buffer_size -= nbyte;
        src = (char *) src + nbyte;
    }

    return nwrite;
}