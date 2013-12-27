#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alpm.h>
#include <alpm_list.h>

typedef struct {
	const char *name;
	int count;
} license_t;

static int limit_output = 1;

char *GPL_LICENSES[20] = {
	"GPL", 
	"GPL2", 
	"GPLv2",
	"GPL3", 
	"GPLv3",
	"LGPL", 
	"LGPL2", 
	"LGPL2.1",
	"LGPL3", 
	"AGPL", 
	NULL
};

static void die(char *err, ...)
{
	va_list e;

	va_start(e, err);
	vfprintf(stderr, err, e);
	va_end(e);

	exit(EXIT_FAILURE);
}

static void print_license_list(alpm_list_t *licenses, int counter)
{
	alpm_list_t *l;
	license_t *license;
	int i = 0;
	int sum = 0;

	for (l = licenses; l; l = l->next) {
		license = (license_t *) l->data;

		printf("+%-8s \n  %05.2f%% (%d/%d)\n", license->name, 
			((double)license->count*100)/counter, license->count, counter);

		/* let's display only the 5 most used license */
		if (limit_output && (i == 4))
			break;

		sum += license->count;
		i++;
	}

	if (limit_output)
		printf("%-8s \n  %05.2f%%  (%d/%d)\n", "+other", 
			((double)(counter-sum)*100)/counter, counter-sum, counter);
}

static void free_list(alpm_list_t *list)
{
	/* free list and data */
	alpm_list_free_inner(list, free);
	alpm_list_free(list);
}

static void *find_license_in_list(alpm_list_t *licenses, char *name)
{
	alpm_list_t *l;
	license_t *license;

	for (l = licenses; l; l = l->next) {
		license = (license_t *) l->data;

		if (!strcmp(name, license->name))
			return license;
	}

	return NULL;
}

static int cmp_count(const void *data1, const void *data2)
{
	/* cast date and return diff */
	return (((license_t *) data2)->count - ((license_t *) data1)->count);
}

static void process_license(alpm_list_t **licenses, char *name)
{
	void *res;

	if ((res = find_license_in_list(*licenses, name))) {
		/* increment counter */
		((license_t *) res)->count += 1;
	} else {
		/* add license to list */
		license_t *license = calloc(1, sizeof(license_t));

		license->name = name;
		license->count = 1;
		*licenses = alpm_list_add(*licenses, license);

	}
}

static alpm_list_t *sort_list(alpm_list_t *list)
{
	alpm_list_t *list_copy = NULL;
	alpm_list_t *l;

	for (l = list; l; l = l->next)
		list_copy = alpm_list_add_sorted(list_copy, l->data, cmp_count);

	/* we get rid of the list, but we keep the data */
	alpm_list_free(list);

	return list_copy;
}

static void usage(char **argv)
{
	die("usage: %s [-a] [-l license] [-v]\n", argv[0]);
}

int main(int argc, char **argv)
{
	const char *root = "/";
	const char *dbpath = "/var/lib/pacman";

	static alpm_handle_t *handle;
	static alpm_db_t *db;

	alpm_list_t *licenses = NULL;
	alpm_list_t *i, *l;

	alpm_errno_t err;
	int counter = 0;

	int find_by_license_flag = 0;

	if ((argc == 2) && !strcmp("-a", argv[1]))
		limit_output = 0;

	else if ((argc == 2) && !strcmp("-v", argv[1]))
		die("%s © 2013\n", "pacfree");

	else if ((argc == 3) && !strcmp("-l", argv[1]))
		find_by_license_flag = 1;

	else if (argc != 1)
		usage(argv);

	handle = alpm_initialize(root, dbpath, &err);

	if (!handle)
		return 1;

	db = alpm_get_localdb(handle);

	for (i = alpm_db_get_pkgcache(db); i; i = alpm_list_next(i)) {
		alpm_pkg_t *pkg = i->data;

		/* packages can have multiple licenses */
		for (l = alpm_pkg_get_licenses(pkg); l; l = l->next) {
			
			if (!find_by_license_flag) {
				process_license(&licenses, (char*) l->data);
				counter++;
			} else {
				if (!strcmp((char*) l->data, argv[2])) {
					/* hulgy hack, we should not reused this structure */
					license_t *license = calloc(1, sizeof(license_t));
					license->name = alpm_pkg_get_name(pkg);
					licenses = alpm_list_add(licenses, license);
					counter++;
				}
			}
		}
	}

	licenses = sort_list(licenses);

	if (!find_by_license_flag) {
		print_license_list(licenses, counter);
	} else {
		if (counter > 0) {
		printf("Found %d packages with license \"%s\":\n", counter, argv[2]);
			for (l = licenses; l; l = l->next) {
				printf("%s\n", ((license_t*) l->data)->name);
			}
		} else {
			printf("No packages found with license \"%s\"\n", argv[2]);
		}
	}

	free_list(licenses);
	alpm_release(handle);

	return 0;
}