/*
 * Copyright (c) 2013, Red Hat Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the
 *       above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or
 *       other materials provided with the distribution.
 *     * The names of contributors to this software may not be
 *       used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Author: Stef Walter <stefw@collabora.co.uk>
 */

#include "config.h"
#include "CuTest.h"

#include "debug.h"
#include "test.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char *
read_file (CuTest *tc,
           const char *file,
           int line,
           const char *filename,
           long *len)
{
	FILE *f = NULL;
	char *data;

	f = fopen (filename, "r");
	if (f == NULL)
		CuFail_Line (tc, file, line, "Couldn't open file", filename);

	/* Figure out size */
	if (fseek (f, 0, SEEK_END) == -1 ||
	    (*len = ftell (f)) == -1 ||
	    fseek (f, 0, SEEK_SET) == -1) {
		CuFail_Line (tc, file, line, "Couldn't get file length", filename);
	}

	data = malloc (*len ? *len : 1);
	assert (data != NULL);

	/* And read in one block */
	if (fread (data, 1, *len, f) != *len)
		CuFail_Line (tc, file, line, "Couldn't read file", filename);

	fclose (f);

	return data;
}

void
test_check_file_msg (CuTest *tc,
                     const char *file,
                     int line,
                     const char *directory,
                     const char *name,
                     const char *reference)
{
	char *refdata;
	long reflen;

	refdata = read_file (tc, file, line, reference, &reflen);
	test_check_data_msg (tc, file, line, directory, name, refdata, reflen);
	free (refdata);
}

void
test_check_data_msg (CuTest *tc,
                     const char *file,
                     int line,
                     const char *directory,
                     const char *name,
                     const void *refdata,
                     long reflen)
{
	char *filedata;
	char *filename;
	long filelen;

	if (asprintf (&filename, "%s/%s", directory, name) < 0)
		CuFail_Line (tc, file, line, "asprintf() failed", NULL);

	filedata = read_file (tc, file, line, filename, &filelen);

	if (filelen != reflen || memcmp (filedata, refdata, reflen) != 0)
		CuFail_Line (tc, file, line, "File contents not as expected", filename);

	unlink (filename);
	free (filename);
	free (filedata);
}

void
test_check_symlink_msg (CuTest *tc,
                        const char *file,
                        int line,
                        const char *directory,
                        const char *name,
                        const char *destination)
{
	char buf[1024] = { 0, };
	char *filename;

	if (asprintf (&filename, "%s/%s", directory, name) < 0)
		CuFail_Line (tc, file, line, "asprintf() failed", NULL);

	if (readlink (filename, buf, sizeof (buf)) < 0)
		CuFail_Line (tc, file, line, "Couldn't read symlink", filename);

	CuAssertStrEquals_LineMsg (tc, file, line, "symlink contents wrong", destination, buf);

	unlink (filename);
	free (filename);
}

p11_dict *
test_check_directory_files (const char *file,
                            ...)
{
	p11_dict *files;
	va_list va;

	files = p11_dict_new (p11_dict_str_hash, p11_dict_str_equal, NULL, NULL);

	va_start (va, file);

	while (file != NULL) {
		if (!p11_dict_set (files, (void *)file, (void *)file))
			return_val_if_reached (NULL);
		file = va_arg (va, const char *);
	}

	va_end (va);

	return files;
}

void
test_check_directory_msg (CuTest *tc,
                          const char *file,
                          int line,
                          const char *directory,
                          p11_dict *files)
{
	p11_dictiter iter;
	struct dirent *dp;
	const char *name;
	DIR *dir;

	dir = opendir (directory);
	if (dir == NULL)
		CuFail_Line (tc, file ,line, "Couldn't open directory", directory);

	while ((dp = readdir (dir)) != NULL) {
		if (strcmp (dp->d_name, ".") == 0 ||
		    strcmp (dp->d_name, "..") == 0)
			continue;

		if (!p11_dict_remove (files, dp->d_name))
			CuFail_Line (tc, file, line, "Unexpected file in directory", dp->d_name);
	}

	closedir (dir);

	p11_dict_iterate (files, &iter);
	while (p11_dict_next (&iter, (void **)&name, NULL))
		CuFail_Line (tc, file, line, "Couldn't find file in directory", name);

	p11_dict_free (files);
}
