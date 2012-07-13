#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define TEST_BLKSZ 4096

union test_handle {
	int fd;
	char *map;
};

int init_map(union test_handle *thd, char *filename, size_t size)
{
	int fd;
	char *map;

	fd = open(filename, O_RDWR);
	if (fd == -1) return -1;

	map = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (map  == MAP_FAILED) {
		close(fd);
		return -1;
	}
	thd->map = map;
	return 0;
}

int init_dio(union test_handle *thd, char *filename, size_t size)
{
	int fd;

	fd = open(filename, O_RDWR|O_DIRECT);
	if (fd == -1) return -1;
	thd->fd = fd;
	return 0;
}

int read_map(union test_handle *thd, off_t offset)
{
	char *map = thd->map;
	char buffer[TEST_BLKSZ];

	memcpy(buffer, map+offset, TEST_BLKSZ);

	return 0;
}

int write_map(union test_handle *thd, off_t offset)
{
	char *map = thd->map;
	char buffer[TEST_BLKSZ];

	memcpy(map+offset, buffer, TEST_BLKSZ);

	return 0;
}

int read_dio(union test_handle *thd, off_t offset)
{
	ssize_t ret;
	int fd = thd->fd;
	char buffer[TEST_BLKSZ];

	ret = pread(fd, buffer, TEST_BLKSZ, offset);
	if (ret != TEST_BLKSZ) return -1;
	return 0;
}

int write_dio(union test_handle *thd, off_t offset)
{
	ssize_t ret;
	int fd = thd->fd;
	char buffer[TEST_BLKSZ];

	ret = pwrite(fd, buffer, TEST_BLKSZ, offset);
	if (ret != TEST_BLKSZ) return -1;
	return 0;
}

/*
int init_test()
{
	srandom(time());
}

int perform_test(long long num)
{
	long long i;
	long int rand;
	for (i=0; i<num; i++) {
		rand = random() & 0xffff;
	}
}
*/

struct test_case {
	int (*init)(union test_handle *, char *, size_t);
	int (*do_test)(union test_handle *, off_t);
};

int simple_test_seq(struct test_case *cs)
{
	int ret;
	int i = 20000;
	off_t offset = 0;
	union test_handle hd;

	ret = cs->init(&hd, "/dev/sdb", ((size_t)1024)*1024*1024*900);
	assert(ret == 0);

	do {
		ret = cs->do_test(&hd, offset);
		assert(ret == 0);
		offset += TEST_BLKSZ;
	} while(i--);

	return 0;
}

int main(int argc, char *argv[])
{
	struct test_case tcs;

	if (argc != 2) {
		printf("%s <d|m>			dio/mmap\n", argv[0]);
		return 0;
	}

	switch (argv[1][0]) {
		case 'd' :
			tcs.init  = init_dio;
			tcs.do_test = write_dio;
			break;
		case 'm' :
			tcs.init  = init_map;
			tcs.do_test = write_map;
			break;
		default :
			printf("%s <d|m>\n", argv[0]);
			return 0;
	};

	simple_test_seq(&tcs);

	return 0;
}
