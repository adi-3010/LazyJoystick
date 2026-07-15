// SPDX-License-Identifier: MIT
/*
 * Copyright © 2021 Red Hat, Inc.
 */

/* Lists all event types and codes currently known by libevdev. */

// taken from libevdev examples. Modified to output to a file for convenience.

#include <stdio.h>
#include <linux/input.h>
#include "libevdev/libevdev.h"
#include <errno.h>
#include <stdlib.h>

static void
list_event_codes(unsigned int type, unsigned int max, FILE* fptr)
{
	const char *typestr = libevdev_event_type_get_name(type);

	if (!typestr)
		return;

	fprintf(fptr, "- %s:\n", typestr);

	for (unsigned int code = 0; code <= max; code++) {
		const char *str = libevdev_event_code_get_name(type, code);

		if (!str)
			continue;

		fprintf(fptr, "    %d: %s\n", code, str);
	}
}

int
main (int argc, char **argv)
{
    FILE *fptr = fopen("event_codes.txt", "a");
    if(fptr == NULL){
        printf("Error opening file");
        return EXIT_FAILURE;
    }
	fprintf(fptr, "codes:\n");
	for (unsigned int type = 0; type <= EV_MAX; type++) {
		int max = libevdev_event_type_get_max(type);
		if (max == -1)
			continue;
		list_event_codes(type, (unsigned int)max, fptr);
	}

	return 0;
}
