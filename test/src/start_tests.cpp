#include <core/parameters.h>
#include <core/dbfile.h>
#include <gtest/gtest.h>
#include <core/row.h>
#include <core/table.h>
#include <unistd.h>

// Demonstrate some basic assertions.

TEST(Row, fromString) {
  UserInfo row;
  row.from_string(" 1 annie annie@google.com");
  EXPECT_STREQ(row.to_string().c_str(), "1,annie,annie@google.com");

  row.from_string(" 2   bob bob@sina.com   ");
  EXPECT_STREQ(row.to_string().c_str(), "2,bob,bob@sina.com");
}

TEST(DbFile, init) {
  DbFile file("./dbfile");
  EXPECT_GE(file.file_descriptor, 0);
  EXPECT_GE(file.length, 0);
}

TEST(DbFile, read_write_buffer) {
  int buffer_size = 1024;
  char * data = new char[buffer_size];
  for (int i=0; i < buffer_size; ++i) {
    data[i] = 'a' + i % 26;
  }

  int fid = open(
    "/tmp/read_write_buffer",
    O_WRONLY | O_CREAT | O_TRUNC,
    S_IWUSR | S_IRUSR
  );

  if (fid < 0) {
    perror("open file error");
    EXPECT_GE(fid, 0);
  }

  // write all data into file
  ssize_t nbyte = write(fid, data, buffer_size);
  EXPECT_EQ(nbyte, buffer_size);
  close(fid);

  // read data from file
  fid = open(
    "/tmp/read_write_buffer",
    O_RDONLY,
    S_IWUSR | S_IRUSR
  );

  if (fid < 0) {
    perror("open file error");
    EXPECT_GE(fid, 0);
  }

  char * buff = new char[buffer_size];
  nbyte = read_buffer(fid, buff, buffer_size);
  EXPECT_EQ(nbyte, buffer_size);
  close(fid);

  for (int i=0; i < buffer_size; ++i)
    EXPECT_EQ(data[i], buff[i]);

  delete []data;
  delete []buff;
}

TEST(DbFile, read_write_page) {
  // write data with two pages into disk
  int n = PAGE_SIZE + 10;
  char * data = new char[n];
  for (int i=0; i < n; ++i) {
    data[i] = 'a';
  }

  int fd = open(
    "/tmp/dbfile",
    O_WRONLY | O_CREAT | O_TRUNC,
    S_IWUSR | S_IRUSR
  );

  write_buffer(fd, data, n);

  delete []data;

  // load pages uisng DbFile
  DbFile * df = new DbFile("/tmp/dbfile");
  void * page0 = df->get_page(0);
  for (int i=0; i < PAGE_SIZE; ++i) {
    EXPECT_EQ(((char *)page0)[i], 'a');
  }

  // load pages 1 pages 2 and write them
  char * page1 = (char *) df->get_page(1);
  for(int i=10; i < PAGE_SIZE; ++i)
    page1[i] = 'a';

  char * page2 = (char *) df->get_page(2);
  for(int i=0; i < 10; ++i)
    page2[i] = 'a';

  df->flush_page(0);
  df->flush_page(1);
  df->flush_page(2, 10);
  delete df;

  df = new DbFile("/tmp/dbfile");
  EXPECT_EQ(df->length, 2*PAGE_SIZE + 10);
  EXPECT_EQ(df->num_pages, 3);

  delete df;
}

TEST(Table, read_and_write_row) {
  int fd = open(
    "/tmp/table",
    O_WRONLY | O_CREAT | O_TRUNC,
    S_IWUSR | S_IRUSR
  );
  close(fd);

  std::string user = "elice";
  std::string email = "elice@google.com";

  UserInfo row(13, user.c_str(), email.c_str());
  Table * tab = new Table(row.get_row_byte(), "/tmp/table");

  size_t num_row_exist = tab->num_rows;
  for (int i=0; i < 5000; ++i) {
    void * slot = tab->get_row_slot(i + num_row_exist);
    row.serialize(slot);
    tab->num_rows += 1;
  }

  printf("num of rows: %ld\n", tab->num_rows);
  delete tab;

  UserInfo newrow;
  tab = new Table(row.get_row_byte(), "/tmp/table");
  for (int i=0; i < 5000; i+=500) {
    void * slot = tab->get_row_slot(i);
    newrow.deserialize(slot);
    EXPECT_EQ(
      newrow.to_string(),
      "13,elice,elice@google.com"
    );
  }
  delete tab;
  // printf("table read and checked\n");
}
