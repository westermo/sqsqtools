/*
 * Copyright (c) 2021  Westermo Network Technologies AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <byteswap.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/magic.h>

#define SQFS_HDR_BYTES_USED     40      /* Position of bytes used in sqfs hdr */
#define SQFS_ALIGN              4096    /* Sqfs default alignment */
#define MAX_SQFS_NUM            256     /* Arbitrary recursion guard */

enum {
	__CMD_ERROR__,
	CMD_LIST, /* default */
	CMD_OFFSET,
	CMD_SIZE,
	CMD_USED,
	CMD_COUNT,
};

int block = 0;

static uint32_t hdr_read_magic(int fd)
{
	uint32_t magic;
	ssize_t n;

	n = read(fd, &magic, sizeof(magic));
	if (n == -1) {
		perror("Error, unable to read magic");
		return 0;
	}
	if (!n)
		return 0;
	if (n != sizeof(magic)) {
		fprintf(stderr, "Error, short read when reading magic\n");
		return 0;
	}

#if __BYTE_ORDER  == __BIG_ENDIAN
	magic = __bswap_32(magic);
#endif
	return magic;
}

static long long hdr_read_bytes_used(int fd)
{
	long long used;
	ssize_t n;

	n = read(fd, &used, sizeof(used));
	if (n == -1) {
		perror("Error, unable to read bytes used");
		return -1;
	}
	if (!n)
		return 0;
	if (n != sizeof(used)) {
		fprintf(stderr, "Error, short read when reading bytes used");
		return -1;
	}

#if __BYTE_ORDER  == __BIG_ENDIAN
	used = __bswap_64(used);
#endif
	return used;
}

static unsigned int calc_sqfs_pad(long long used)
{
	unsigned int tail;

	tail = used % SQFS_ALIGN;

	return SQFS_ALIGN - tail;
}

static void print_sqfs_info(long long used, size_t offset, int num)
{
	int fs;

	fs = (num % 2) ? num / 2 : num / 2 - 1;

	if (num % 2 != 0)
		printf("%d: Filesystem%02d     ", num, fs + 1);
	else
		printf("%d: Filesystem%02d-meta", num, fs + 1);

	if (block) {
		printf(" (%llu blocks @ block %zu)\n",
		       used / SQFS_ALIGN ? : 1, offset / SQFS_ALIGN);
	} else {
		printf(" (%llu bytes @ %zu)\n", used, offset);
	}
}

/* num has to start at 1 */
static int foreach_sqfs(int fd, off_t offset, int num, int cmd, int filter)
{
	unsigned int pad;
	long long used;
	uint32_t magic;

	/* Set fd at sqfs start */
	if (lseek(fd, offset, SEEK_SET) == -1) {
		perror("Error, seeking in binary file");
		return -1;
	}

	/* Recursion guard */
	if (num > MAX_SQFS_NUM) {
		fprintf(stderr, "Error, to many squash filesystems found\n");
		return -1;
	}

	magic = hdr_read_magic(fd);

	/* We expect this to happen at the end of the file */
	if (!magic || magic != SQUASHFS_MAGIC)
		return num - 1;

	if (lseek(fd, SQFS_HDR_BYTES_USED - sizeof(magic), SEEK_CUR) == -1) {
		perror("Error, seeking in binary file");
		return 1;
	}

	used = hdr_read_bytes_used(fd);
	if (used < 0) {
		fprintf(stderr, "Error, reading bytes used\n");
		return 1;
	}
	pad = calc_sqfs_pad(used);

	if (cmd == CMD_COUNT) {
		if (filter) {
			fprintf(stderr, "Error, can't use filter with count\n");
			return 1;
		}
		goto next;
	}

	if (!filter || filter == num) {
		switch (cmd) {
		case CMD_LIST:
			print_sqfs_info(used, offset, num);
			break;
		case CMD_OFFSET:
			printf("%jd\n", block ? (intmax_t)offset / SQFS_ALIGN :
						(intmax_t)offset);
			break;
		case CMD_SIZE:
			printf("%llu\n", block ? (used + pad) / SQFS_ALIGN :
						 used + pad);
			break;
		case CMD_USED:
			printf("%llu\n", block ? used / SQFS_ALIGN : used);
			break;
		}
	}

next:
	return foreach_sqfs(fd, offset + used + pad, ++num, cmd, filter);
}

static int usage(char *argv0)
{
	fprintf(stderr, "Usage: %s [OPTIONS] [COMMAND] SQSQ-IMAGE\n", argv0);
	fprintf(stderr, "\nCommands:\n");
	fprintf(stderr, "  list   - List filesystems (default)\n");
	fprintf(stderr, "  offset - Get start address of filesystem\n");
	fprintf(stderr, "  size   - Get padded size of filesystem\n");
	fprintf(stderr, "  used   - Get squashfs BYTES_USED of filesystem\n");
	fprintf(stderr, "  count  - Count the number of filesystems\n");

	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "  -n NUMBER  - Only look at filesystem NUMBER\n");
	fprintf(stderr, "  -b         - Count in sqfs blocks (4096 bytes)\n");
	fprintf(stderr, "  -h         - Help (this text)\n");

	return 1;
}

static int parse_cmd(const char *word)
{
	if (strcmp("list", word) == 0)
		return CMD_LIST;
	if (strcmp("offset", word) == 0)
		return CMD_OFFSET;
	if (strcmp("size", word) == 0)
		return CMD_SIZE;
	if (strcmp("used", word) == 0)
		return CMD_USED;
	if (strcmp("count", word) == 0)
		return CMD_COUNT;

	return __CMD_ERROR__;
}

int main(int argc, char *argv[])
{
	const char *filename;
	int filter;
	int cmd;
	int num;
	int opt;
	int fd;

	filter = 0;
	cmd = CMD_LIST;

	while ((opt = getopt(argc, argv, "hbn:")) != -1) {
		switch (opt) {
		case 'n':
			filter = atoi(optarg);
			break;
		case 'b':
			block = 1;
			break;
		case 'h':
			return usage(argv[0]);
		}
	}

	if (argc - optind > 1) {
		filename = argv[optind + 1];

		cmd = parse_cmd(argv[optind]);
		if (!cmd) {
			fprintf(stderr, "Error, invalid operation \"%s\"\n",
				argv[optind]);
			return 1;
		}
	} else if (argc - optind == 1) {
		filename = argv[optind];
	} else {
		return usage(argv[0]);
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("Error, opening file");
		return 1;
	}

	num = foreach_sqfs(fd, 0, 1, cmd, filter);
	if (num < 0) {
		fprintf(stderr, "Error, searching for sqfs in image\n");
		goto out;
	}
	if (cmd == CMD_COUNT)
		printf("%d\n", num);
	else if (cmd == CMD_LIST && !filter)
		fprintf(stderr, "\nFound %d filesystems in image\n", num);

out:
	close(fd);

	return (num >= 2) ? 0 : 1;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
