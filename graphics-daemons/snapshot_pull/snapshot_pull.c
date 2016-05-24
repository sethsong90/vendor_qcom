/* -------------------------------------------------------------------------
 *
 * Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 *//*----------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <cutils/log.h>

#define DEFAULT_SNAPSHOT_DIR "/sys/class/kgsl/kgsl-3d0/snapshot"
#define DEFAULT_SNAPSHOT_SAVE_DIR "/data/local/tmp/gpu_snapshot-3d0"

#define TIMESTAMP_FILE "timestamp"
#define DUMP_FILE "dump"

#define BUF_MAXSIZE 4096

static char dump_save_fname[128];
static char ts_read_fname[128];
static char dump_read_fname[128];
static char snapshot_src_dir[128];
static char snapshot_dest_dir[128];

static int parse_args(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "s:d:")) != -1) {
		switch (opt) {
		case 's':
			snprintf(snapshot_src_dir,
				sizeof(snapshot_src_dir), "%s",
				optarg);
			break;
		case 'd':
			snprintf(snapshot_dest_dir,
				sizeof(snapshot_dest_dir), "%s",
				optarg);
			break;
		default:
			ALOGE("Usage: %s [-s src_dir] [-d dest_dir]",
				argv[0]);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int ts_fd;
	int read_dump_fd;
	int write_dump_fd;
	int read_bytes;
	int written_bytes = 0;
	int ret;
	int count = 0;
	char timestamp_str[10]; /* max 8 chars + '\n' + '\0' */
	unsigned int timestamp = 0;
	unsigned int prev_timestamp = 0;
	struct pollfd fds[1];
	static unsigned char buffer[BUF_MAXSIZE];

	snprintf(snapshot_src_dir, sizeof(snapshot_src_dir), "%s",
			DEFAULT_SNAPSHOT_DIR);
	snprintf(snapshot_dest_dir, sizeof(snapshot_dest_dir), "%s",
			DEFAULT_SNAPSHOT_SAVE_DIR);

	if (parse_args(argc, argv) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	ALOGI("src dir: %s dest dir: %s\n", snapshot_src_dir, snapshot_dest_dir);

	snprintf(ts_read_fname, sizeof(ts_read_fname), "%s/%s",
			snapshot_src_dir, TIMESTAMP_FILE);
	snprintf(dump_read_fname, sizeof(dump_read_fname), "%s/%s",
			snapshot_src_dir, DUMP_FILE);

	/* Make sure the dest dir exists */
	ret = mkdir(snapshot_dest_dir, 0755);
	if ((ret < 0) && (errno != EEXIST)) {
		ALOGE("Can't create the output dir %s: %s", snapshot_dest_dir,
			strerror(errno));
		return EXIT_FAILURE;
	}

	while (1) {
		/* Read the timstamp file, then poll for POLLERR | POLLPRI
		 * This returns when sysfs_notify() is called in kernelspace
		 * on the timestamp attribute
		 */
		ts_fd = open(ts_read_fname, O_RDONLY);
		if (ts_fd < 0) {
			ALOGE("Can't open %s: %s", ts_read_fname,
					strerror(errno));
			return EXIT_FAILURE;
		}
		/* timestamp is in ASCII text */
		read_bytes = read(ts_fd, timestamp_str,
				sizeof(timestamp_str) - 1);
		if (read_bytes < 0) {
			ALOGE("Can't read %s: %s", ts_read_fname,
					strerror(errno));
			return EXIT_FAILURE;
		}
		timestamp_str[read_bytes] = 0;
		timestamp = strtoul(timestamp_str, NULL, 16);

		if (timestamp != prev_timestamp) {
			/* Append timestamp to the end of the filenames
			 * in case of several hangs */
			snprintf(dump_save_fname, sizeof(dump_save_fname),
				"%s/%s_%08x", snapshot_dest_dir, DUMP_FILE,
				timestamp);
			prev_timestamp = timestamp;

			ALOGI("GPU hang at %x: copying dump to %s",
					timestamp, snapshot_dest_dir);
			read_dump_fd = open(dump_read_fname, O_RDONLY);
			if (read_dump_fd < 0) {
				ALOGE("Can't open %s: %s",
					dump_read_fname, strerror(errno));
				return EXIT_FAILURE;
			}
			write_dump_fd = open(dump_save_fname,
					O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (write_dump_fd < 0) {
				ALOGE("Can't open %s: %s", dump_save_fname,
					strerror(errno));
				return EXIT_FAILURE;
			}
			/* Copy the entire dump file */
			do {
				read_bytes = read(read_dump_fd, buffer,
							BUF_MAXSIZE);
				if (read_bytes > 0)
					written_bytes = write(write_dump_fd,
							buffer, read_bytes);
			} while (((read_bytes > 0) &&
					(read_bytes == written_bytes)) ||
					(errno == EAGAIN));
			close(write_dump_fd);
			close(read_dump_fd);
			/* If there was an error reading the input or writing
			 * to the output file, warn but keep the data.
			 * Might still be useful */
			if (read_bytes < 0)
				ALOGW("Error while reading %s: %s",
					dump_read_fname, strerror(errno));
			else if (written_bytes < 0)
				ALOGW("Error while writing to %s: %s",
					dump_save_fname, strerror(errno));
			else if (written_bytes < read_bytes)
				ALOGW("Data not completely written!");
			else
				ALOGI("GPU snapshot written to %s",
					dump_save_fname);
		}

		fds[0].fd = ts_fd;
		fds[0].events = POLLERR | POLLPRI;
		fds[0].revents = 0;
		/* Wait with infinite timeout */
		ret = poll(fds, 1, -1);
		if (ret < 0) {
			ALOGE("Can't poll on %s: %s",
				ts_read_fname, strerror(errno));
			return EXIT_FAILURE;
		}
		close(ts_fd);
	}
	return EXIT_SUCCESS;
}

