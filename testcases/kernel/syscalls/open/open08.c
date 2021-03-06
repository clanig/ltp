/*
 *   Copyright (c) 2013 Wanlong Gao <gaowanlong@cn.fujitsu.com>
 *   Copyright (c) 2018 Linux Test Project
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * DESCRIPTION
 *	Check for the following errors:
 *	1.	EEXIST
 *	2.	EISDIR
 *	3.	ENOTDIR
 *	4.	ENAMETOOLONG
 *	5.	EACCES
 *	6.	EFAULT
 *
 * ALGORITHM
 *	1. Open a file with O_CREAT and O_EXCL, when the file already
 *	   exists. Check the errno for EEXIST
 *
 *	2. Pass a directory as the pathname and request a write access,
 *	   check for errno for EISDIR
 *
 *	3. Specify O_DIRECTORY as a parameter to open and pass a file as the
 *	   pathname, check errno for ENOTDIR
 *
 *	4. Attempt to open() a filename which is more than VFS_MAXNAMLEN, and
 *	   check for errno to be ENAMETOOLONG.
 *
 *	5. Attempt to open a (0600) file owned by different user in WRONLY mode,
 *	   open(2) should fail with EACCES.
 *
 *	6. Attempt to pass an invalid pathname with an address pointing outside
 *	   the accessible address space of the process, as the argument to open(),
 *	   and expect to get EFAULT.
 */

#define _GNU_SOURCE		/* for O_DIRECTORY */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include "tst_test.h"
#include "tst_get_bad_addr.h"

static char *existing_fname = "open08_testfile";
static char *toolong_fname = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstmnopqrstuvwxyzabcdefghijklmnopqrstmnopqrstuvwxyzabcdefghijklmnopqrstmnopqrstuvwxyzabcdefghijklmnopqrstmnopqrstuvwxyzabcdefghijklmnopqrstmnopqrstuvwxyzabcdefghijklmnopqrstmnopqrstuvwxyz";
static char *dir_fname = "/tmp";
static char *user2_fname = "user2_0600";
static char *unmapped_fname;

struct test_case_t;

static struct test_case_t {
	char **fname;
	int flags;
	int error;
} tcases[] = {
	{&existing_fname, O_CREAT | O_EXCL, EEXIST},
	{&dir_fname, O_RDWR, EISDIR},
	{&existing_fname, O_DIRECTORY, ENOTDIR},
	{&toolong_fname, O_RDWR, ENAMETOOLONG},
	{&user2_fname, O_WRONLY, EACCES},
	{&unmapped_fname, O_CREAT, EFAULT}
};

void verify_open(unsigned int i)
{
	TEST(open(*tcases[i].fname, tcases[i].flags,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

	if (TEST_RETURN != -1) {
		tst_res(TFAIL, "call succeeded unexpectedly");
		return;
	}

	if (TEST_ERRNO == tcases[i].error) {
		tst_res(TPASS, "expected failure - "
				"errno = %d : %s", TEST_ERRNO,
				strerror(TEST_ERRNO));
	} else {
		tst_res(TFAIL, "unexpected error - %d : %s - "
				"expected %d", TEST_ERRNO,
				strerror(TEST_ERRNO), tcases[i].error);
	}
}

static void setup(void)
{
	int fildes;
	char nobody_uid[] = "nobody";
	struct passwd *ltpuser;

	umask(0);

	SAFE_CREAT(user2_fname, 0600);

	/* Switch to nobody user for correct error code collection */
	ltpuser = getpwnam(nobody_uid);
	SAFE_SETGID(ltpuser->pw_gid);
	SAFE_SETUID(ltpuser->pw_uid);

	fildes = SAFE_CREAT(existing_fname, 0600);
	close(fildes);

	unmapped_fname = tst_get_bad_addr(NULL);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.needs_tmpdir = 1,
	.needs_root = 1,
	.setup = setup,
	.test = verify_open,
};
