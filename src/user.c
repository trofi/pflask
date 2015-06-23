/*
 * The process in the flask.
 *
 * Copyright (c) 2013, Alessandro Ghedini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <pwd.h>
#include <grp.h>

#include "printf.h"
#include "util.h"

void map_user_to_user(uid_t uid, gid_t gid, const char *user, pid_t pid) {
	int rc;

	_free_ char *setgroups_file = NULL;
	_free_ char *uid_map_file = NULL;
	_free_ char *gid_map_file = NULL;

	_close_ int setgroups_fd = -1;
	_close_ int uid_map_fd = -1;
	_close_ int gid_map_fd = -1;

	_free_ char *uid_map = NULL;
	_free_ char *gid_map = NULL;

	uid_t pw_uid;
	uid_t pw_gid;

	struct passwd *pwd;

	if (strncmp(user, "root", 5) == 0) {
		pw_uid = 0;
		pw_gid = 0;
	} else {
		errno = 0;
		pwd = getpwnam(user);
		if (pwd == NULL) {
			if (errno) sysf_printf("getpwnam()");
			else       fail_printf("Invalid user '%s'", user);
		}

		pw_uid = pwd->pw_uid;
		pw_gid = pwd->pw_gid;
	}

	/* setgroups */
	rc = asprintf(&setgroups_file, "/proc/%d/setgroups", pid);
	if (rc < 0) fail_printf("OOM");

	setgroups_fd = open(setgroups_file, O_RDWR);
	if (setgroups_fd >= 0) {
		rc = write(setgroups_fd, "deny", 4);
		if (rc < 0) sysf_printf("write(setgroups)");
	}

	/* uid map */
	rc = asprintf(&uid_map_file, "/proc/%d/uid_map", pid);
	if (rc < 0) fail_printf("OOM");

	rc = asprintf(&uid_map, "%d %d 1", pw_uid, uid);
	if (rc < 0) fail_printf("OOM");

	uid_map_fd = open(uid_map_file, O_RDWR);
	if (uid_map_fd < 0) sysf_printf("open(uid_map)");

	rc = write(uid_map_fd, uid_map, strlen(uid_map));
	if (rc < 0) sysf_printf("write(uid_map)");

	/* gid map */
	rc = asprintf(&gid_map_file, "/proc/%d/gid_map", pid);
	if (rc < 0) fail_printf("OOM");

	rc = asprintf(&gid_map, "%d %d 1", pw_gid, gid);
	if (rc < 0) fail_printf("OOM");

	gid_map_fd = open(gid_map_file, O_RDWR);
	if (gid_map_fd < 0) sysf_printf("open(gid_map)");

	rc = write(gid_map_fd, gid_map, strlen(gid_map));
	if (rc < 0) sysf_printf("write(gid_map)");
}

void setup_user(const char *user) {
	int rc;

	uid_t pw_uid;
	uid_t pw_gid;

	struct passwd *pwd;

	if (strncmp(user, "root", 5) == 0) {
		pw_uid = 0;
		pw_gid = 0;
	} else {
		errno = 0;
		pwd = getpwnam(user);
		if (pwd == NULL) {
			if (errno) sysf_printf("getpwnam()");
			else       fail_printf("Invalid user '%s'", user);
		}

		pw_uid = pwd->pw_uid;
		pw_gid = pwd->pw_gid;
	}

	rc = setresgid(pw_gid, pw_gid, pw_gid);
	if (rc < 0) sysf_printf("setresgid()");

	rc = setresuid(pw_uid, pw_uid, pw_uid);
	if (rc < 0) sysf_printf("setresuid()");
}
