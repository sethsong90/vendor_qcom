/*
 * User-space unit test application for the bma150 / SPI driver.
 *
 * Copyright (c) 2008 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <sys/types.h>
#include <ctype.h>
#include <linux/input.h>

#define G_CONVERT                     (1.0/128)
#define ASCII_DEGREE                  248
#define COUNT_LIMIT                   500
#define TEST_REG                      0x12

#define REG10_ANY_MOTION_THRESH_VALUE 0x02
#define REG14_RANGE_MASK              0x18
#define REG14_RANGE_VALUE	      0x08
#define REG14_BANDWIDTH_MASK          0x07
#define REG14_BANDWIDTH_VALUE	      0x04
#define REG15_ENABLE_ADV_INT_MASK     0x40
#define REG15_ENABLE_ADV_INT_VALUE    0x40
#define REG15_LATCH_INT_MASK          0x10
#define REG15_LATCH_INT_VALUE         0x10
#define REG15_SHADOW_DIS_MASK         0x08
#define REG15_SHADOW_DIS_VALUE        0x00

struct bma150_accel_data {
	int              accel_x;
	int              accel_y;
	int              accel_z;
	int              temp;
};

static void usage(void)
{
	printf("Syntax: spitest device test_number\n");
	printf("device is the device name for the bma150 accelerometer\n");
	printf("in the /dev directory, for example: event1\n");
	printf("Note that the spitest.sh script determines this "
	       "automatically.\n\n");
	printf("The currently implemented tests are:\n");
	printf("\n");
	printf("Test number	Test \n");
	printf("    1		Verify accelerometer is recognized\n");
	printf("    2		Verify no failure if accelerometer is not "
	       "connected\n");
	printf("    3		Verify data read from accelerometer\n");
	printf("    4		Verify temperature data from accelerometer\n");
	printf("    5		Verify stale data not returned\n");
	printf("    6		Verify different bytes per word SPI "
	       "transactions\n");
	printf("    7		Verify large data transfers\n");
}

static int dev_open(char *dev_name)
{
	int fp;
	char buf[50];

	snprintf(buf, sizeof(buf), "/dev/%s", dev_name);
	fp = open(buf, O_RDWR);
	if (fp <= 0)
		printf("Could not open device %s\n", buf);
	return(fp);
}

static int dev_reg_open(char *debugfs_dev_name)
{
	int fpr;
	char buf[50];

	snprintf(buf, sizeof(buf), "/debug/%s/registers", debugfs_dev_name);
	fpr = open(buf, O_RDWR);
	if (fpr <= 0)
		printf("Could not open device %s\n", buf);
	return(fpr);
}

static int dev_bpw_open(char *debugfs_dev_name)
{
	int fpb;
	char buf[50];

	snprintf(buf, sizeof(buf), "/debug/%s/bytes_per_word",
		 debugfs_dev_name);
	fpb = open(buf, O_RDWR);
	if (fpb <= 0)
		printf("Could not open device %s\n", buf);
	return(fpb);
}

static int read_packet(struct bma150_accel_data *acc, int fp)
{
	int                       rc = 0;
	int                       size;
	struct input_event        event;
	int                       data_received;

	/* input subsystem doesn't send relative event message */
	/* if value is zero, so init fields.                   */
	acc->accel_x = 0;
	acc->accel_y = 0;
	acc->accel_z = 0;
	acc->temp    = 0;
	/* Sometimes we get a sync with no data preceding it.  */
	/* Don't want to return 0 data, so track when some     */
	/* data has been received.                             */
	data_received = 0;
	do {
		size = read(fp, &event, sizeof(event));
		if (size != sizeof(event)) {
			printf("Error reading data\n");
			printf("expected %d, got rc=%d\n", sizeof(event), size);
			rc = 1;
			goto rd_packet_exit;
		}

		if (event.type == EV_REL) {
			data_received = 1;
			switch (event.code) {
			case REL_X:
				acc->accel_x = event.value;
				break;
			case REL_Y:
				acc->accel_y = event.value;
				break;
			case REL_Z:
				acc->accel_z = event.value;
				break;
			case REL_MISC:
				acc->temp = event.value;
				break;
			}
		}
	} while ((event.type != EV_SYN) || !data_received);
rd_packet_exit:
	return rc;
}

static int device_present(char *dev_name)
{
	int rc = 0;
	int fp;

	fp = dev_open(dev_name);
	if (fp <= 0) {
		rc = 1;
		goto dp_exit;
	}

	close(fp);
dp_exit:
	return rc;
}

static int device_not_present(char *dev_name)
{
	int rc = 0;
	int fp;
	char buf[50];

	snprintf(buf, sizeof(buf), "/dev/%s", dev_name);
	fp = open(buf, O_RDWR);
	if (fp > 0) {
		printf("Device %s detected when it should not be.\n", buf);
		printf("Confirm bma150 is not connected to system at boot "
		       "time.\n");
		rc = 1;
		close(fp);
	}
	return rc;
}

static int read_data(char *dev_name, char *debugfs_dev_name)
{
	int                       rc = 0;
	int                       count;
	int                       fp;
	float                     x_g, y_g, z_g;
	struct bma150_accel_data  acc;

	fp = dev_open(dev_name);
	if (fp <= 0) {
		rc = 1;
		goto rd_exit;
	}

	x_g = 0.0;
	y_g = 0.0;
	x_g = 0.0;

	printf("Move accelerometer, verify data matches movement\n");
	printf("Exits after %d readings\n", COUNT_LIMIT);
	for (count = 0; count <= COUNT_LIMIT; count++) {
		rc = read_packet(&acc, fp);
		if (rc)
			goto rd_conf_exit;
		x_g = acc.accel_x * G_CONVERT;
		y_g = acc.accel_y * G_CONVERT;
		z_g = acc.accel_z * G_CONVERT;
		printf("%03d: x=%.3fg y=%.3fg z=%.3fg\n", count, x_g, y_g, z_g);
	}

rd_conf_exit:
	close(fp);
rd_exit:
	return rc;
}

static int read_temp(char *dev_name, char *debugfs_dev_name)
{
	int                       rc = 0;
	int                       fp;
	int                       last_temp;
	float                     orig_temp_f;
	float                     temp_f;
	struct bma150_accel_data  acc;

	fp = dev_open(dev_name);
	if (fp <= 0) {
		rc = 1;
		goto rt_exit;
	}

	printf("Increase accelerometer temperature\n");
	printf("Exits after change of 2 degrees F\n");
	rc = read_packet(&acc, fp);
	if (rc) {
		printf("Error reading data\n");
		goto rt_conf_exit;
	}
	last_temp = acc.temp;
	orig_temp_f = (acc.temp*1.8)/2+32;
	printf("temp = %.1f%cF\n", orig_temp_f, ASCII_DEGREE);
	do {
		rc = read_packet(&acc, fp);
		if (rc) {
			printf("Error reading data\n");
			goto rt_conf_exit;
		}
		if ((acc.temp > last_temp) || (acc.temp < (last_temp - 2))) {
			temp_f = (acc.temp*1.8)/2+32;
			printf("temp = %.1f%cF, change from start = %.1f%cF\n",
			       temp_f, ASCII_DEGREE,
			       temp_f - orig_temp_f, ASCII_DEGREE);
			last_temp = acc.temp;
		}
	} while ((temp_f - orig_temp_f) < 2.0);

rt_conf_exit:
	close(fp);
rt_exit:
	return rc;
}

static int no_stale_data(char *dev_name, char *debugfs_dev_name)
{
	int                       rc = 0;
	int                       fp;
	int                       temp, temp_save;
	int                       count;
	float                     x_g, y_g, z_g;
	float                     x_g_save, y_g_save, z_g_save;
	struct bma150_accel_data  acc;

	fp = dev_open(dev_name);
	if (fp <= 0) {
		rc = 1;
		goto nsd_exit;
	}

	printf("Move accelerometer, verify many readings occur.\n");
	printf("Leave accelerometer still. Verify readings stop\n");
	printf("(or reduced to minimal level). Test also verifies\n");
	printf("no duplicate consecutive readings due to stale data.\n");
	printf("Exits after %d readings\n", COUNT_LIMIT);
	rc = read_packet(&acc, fp);
	if (rc) {
		printf("Error reading data\n");
		goto nsd_conf_exit;
	}
	x_g_save = acc.accel_x * G_CONVERT;
	y_g_save = acc.accel_y * G_CONVERT;
	z_g_save = acc.accel_z * G_CONVERT;
	temp_save = acc.temp;
	count = 0;
	printf("%03d: x=%.3fg y=%.3fg z=%.3fg\n", count,
	       x_g_save, y_g_save, z_g_save);
	for (count = 1; count <= COUNT_LIMIT; count++) {
		rc = read_packet(&acc, fp);
		if (rc) {
			printf("Error reading data\n");
			goto nsd_conf_exit;
		}
		x_g = acc.accel_x * G_CONVERT;
		y_g = acc.accel_y * G_CONVERT;
		z_g = acc.accel_z * G_CONVERT;
		printf("%03d: x=%.3fg y=%.3fg z=%.3fg\n", count,
		       x_g, y_g, z_g);
		if ((x_g == x_g_save) && (y_g == y_g_save) &&
		    (z_g == z_g_save) && (acc.temp == temp_save)) {
			printf("error: duplicate values detected\n");
			rc = 1;
			goto nsd_conf_exit;
		}
		x_g_save = x_g;
		y_g_save = y_g;
		z_g_save = z_g;
		temp_save = temp;
	}

nsd_conf_exit:
	close(fp);
nsd_exit:
	return rc;
}

/* bytes_per_word test sets the bytes_per_word parameter to values */
/* from 1 to 4. For each of these it does a write of count values  */
/* to the test register. It reads back the final value written     */
/* to confirm it is correct.                                       */
static int bytes_per_word_test(char *debugfs_dev_name, int count)
{
	int                         rc = 0;
	int                         fpb, fpr;
	int                         bytes_per_word;
	int                         write_size;
	int                         error_count;
	int                         i;
	int                         wr_size;
	int                         wrlen;
	int                         bytes_left;
	unsigned char               saved_reg;
	unsigned char               saved_bpw;
	unsigned char               reg;
	char                       *endp;
	char                        buf[8];
	char                       *wr;
	char                       *p;

	error_count = 0;

	/* the format of the write to registers file is: "rr dd ", */
	/* where rr is the register number and dd is the data,     */
	/* both in hex, but with no leading "0x". So each write to */
	/* a register takes 6 bytes of formatted data.             */
	wr_size = count * 6;
	wr = malloc(wr_size);
	if (!wr) {
		error_count++;
		goto bpw_exit;
	}
	fpb = dev_bpw_open(debugfs_dev_name);
	if (fpb <= 0) {
		error_count++;
		goto bpw_exit_0;
	}
	fpr = dev_reg_open(debugfs_dev_name);
	if (fpr <= 0) {
		error_count++;
		goto bpw_exit_1;
	}

	lseek(fpr, TEST_REG, SEEK_SET);
	rc = read(fpr, buf, sizeof(buf));
	if (rc < 0) {
		printf("error: read register 0x%x failed\n", TEST_REG);
		error_count++;
		goto bpw_conf_exit;
	}
	saved_reg = strtoul(buf, &endp, 16);

	rc = read(fpb, buf, sizeof(buf));
	if (rc < 0) {
		printf("error: read bytes_per_word failed\n");
		error_count++;
		goto bpw_conf_exit;
	}
	saved_bpw = strtoul(buf, &endp, 16);

	for (bytes_per_word = 1; bytes_per_word <= 4; bytes_per_word++) {
		snprintf(buf, sizeof(buf), "%d", bytes_per_word);
		rc = write(fpb, buf, strlen(buf));
		if (rc <= 0) {
			printf("error: write for %d bytes per word failed\n",
			       bytes_per_word);
			error_count++;
			continue;
		}
		/* This performs a multi-byte transaction by creating a */
		/* register write command which repeatedly writes to    */
		/* the test register                                    */
		p = wr;
		bytes_left = wr_size;
		for (i = 0; i < (count-1); i++) {
			wrlen = snprintf(p, wr_size, "%x %x ", TEST_REG, 0);
			p += wrlen;
			bytes_left -= wrlen;
		}
		/* On the final write set the TEST_REG to unique value */
		wrlen = snprintf(p, wr_size, "%x %x ", TEST_REG,
				 bytes_per_word);
		p += wrlen;
		write_size = p - wr;
		rc = write(fpr, wr, write_size);
		if (rc != write_size) {
			printf("error during write, bytes per word = %d, "
			       "rc=%d write_size=%d\n",
			       bytes_per_word, rc, write_size);
			error_count++;
		}
		/* confirm the final write worked */
		lseek(fpr, TEST_REG, SEEK_SET);
		rc = read(fpr, buf, sizeof(buf));
		if (rc < 0) {
			printf("error: read register %d (bpw=%d) failed\n",
			       TEST_REG, bytes_per_word);
			error_count++;
			continue;
		}
		reg = strtoul(buf, &endp, 16);
		if (reg != bytes_per_word) {
			printf("bad data in reg 0x%x: expected 0x%x read "
			       "0x%x\n",
			       TEST_REG, bytes_per_word, reg);
			error_count++;
			continue;
		}
	}

	/* restore TEST_REG */
	wrlen = snprintf(wr, wr_size, "%x %x", TEST_REG,
			 saved_reg);
	write_size = wrlen;
	rc = write(fpr, wr, write_size);
	if (rc != write_size) {
		printf("error during write of register 0x%x\n", TEST_REG);
		error_count++;
	}

	/* restore bytes_per_word */
	snprintf(buf, sizeof(buf), "%d", saved_bpw);
	rc = write(fpb, buf, strlen(buf));
	if (rc <= 0) {
		printf("error during write of bytes_per_word\n");
		error_count++;
	}

bpw_conf_exit:
	close(fpr);
bpw_exit_1:
	close(fpb);
bpw_exit_0:
	free(wr);
bpw_exit:
	return error_count;
}

int main(int argc, char **argv)
{
	int test_number;
	char *device_name;
	char *debugfs_device_name = "bma150-0.0";

	if (!argv[2]) {
		usage();
		return(1);
	}
	device_name = argv[1];
	test_number = atoi(argv[2]);

	printf("Running test %d\n", test_number);

	switch (test_number) {
	case 1:
		return(device_present(device_name));
		break;
	case 2:
		return(device_not_present(device_name));
		break;
	case 3:
		return(read_data(device_name, debugfs_device_name));
		break;
	case 4:
		return(read_temp(device_name, debugfs_device_name));
		break;
	case 5:
		return(no_stale_data(device_name, debugfs_device_name));
		break;
	case 6:
		return(bytes_per_word_test(debugfs_device_name, 2));
		break;
	case 7:
		return(bytes_per_word_test(debugfs_device_name, 72));
		break;
	default:
		printf("Unknown test\n");
		usage();
		break;
	}
	return(1);
}
