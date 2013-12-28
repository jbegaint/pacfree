#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alpm.h>
#include <alpm_list.h>
	
#define ROOT "/"
#define PATH "/var/lib/pacman"

typedef struct {
	const char *name;
	int count;
} license_t;

/* unused for now */
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

static void print_licenses_list(alpm_list_t *licenses, int counter, 
	char limit_output)
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

static void *get_license_in_list(alpm_list_t *licenses, char *name)
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

static int cmp_count(const void *data1, const void *data2) {     
	/* cast data and return diff */     
	return (((license_t *) data2)->count - ((license_t *) data1)->count); 
}

static void process_license(alpm_list_t **licenses, char *name)
{
	void *res;

	if ((res = get_license_in_list(*licenses, name))) {
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
	alpm_list_t *l;
	alpm_list_t *sorted_list = NULL;

	for (l = list; l; l = l->next)
		sorted_list = alpm_list_add_sorted(sorted_list, l->data, cmp_count);

	/* we get rid of the list, but we keep the data */
	alpm_list_free(list);

	return sorted_list;
}

alpm_db_t* init_db(alpm_handle_t **handle)
{
	alpm_db_t *db;
	alpm_errno_t err;

	*handle = alpm_initialize(ROOT, PATH, &err);

	if (!*handle)
		die("Error: %s", alpm_strerror(err));

	db = alpm_get_localdb(*handle);

	return db;
}

static void get_licenses(alpm_handle_t **handle, char limit_output)
{
	alpm_db_t *db;
	alpm_list_t *i, *l;
	alpm_list_t *licenses = NULL;
	int counter = 0;

	db = init_db(handle);

	for (i = alpm_db_get_pkgcache(db); i; i = i->next) {
		alpm_pkg_t *pkg = i->data;

		/* packages can have multiple licenses */
		for (l = alpm_pkg_get_licenses(pkg); l; l = l->next) {
			/* add license to list or increment license counter */
			process_license(&licenses, (char*) l->data);
			counter++;
		}
	}

	licenses = sort_list(licenses);
	print_licenses_list(licenses, counter, limit_output);

	free_list(licenses);
}

static void get_pkgs_by_license(alpm_handle_t **handle, char *name)
{
	alpm_db_t *db;
	alpm_list_t *i, *l;
	alpm_pkg_t *pkg;
	const char *pkg_name;
	alpm_list_t *pkgs = NULL;
	int counter = 0;

	db = init_db(handle);

	for (i = alpm_db_get_pkgcache(db); i; i = i->next) {
		pkg = (alpm_pkg_t*) i->data;

		for (l = alpm_pkg_get_licenses(pkg); l; l = l->next) {
			if (!strcmp((char*) l->data, name)) {
				pkg_name = alpm_pkg_get_name(pkg);
				pkgs = alpm_list_add(pkgs, (void*) pkg_name);
				counter++;
			}
		}
	}

	if (counter > 0) {
		printf("Found %d package%s with license \"%s\":\n", counter, 
			(counter > 1) ? "s" : "", name);

		for (l = pkgs; l; l = l->next)
			printf("%s\n", (char*) l->data);
	
	} else {
		printf("No packages found with license \"%s\"\n", name);
	}

	alpm_list_free(pkgs);
}

static void usage(char **argv)
{
	die("usage: %s [-a] [-l license] [-v]\n", argv[0]);
}

int main(int argc, char **argv)
{
	static alpm_handle_t *handle;

	if ((argc == 2) && !strcmp("-v", argv[1])) {
		die("%s ©2013 jeanbroid\n", "pacfree");
	}
	else if ((argc == 2) && !strcmp("-a", argv[1])) {
		get_licenses(&handle, 0);
	}
	else if ((argc == 3) && !strcmp("-l", argv[1])) {
 		get_pkgs_by_license(&handle, argv[2]);
	}
	else if (argc != 1) {
		usage(argv);
	}
	else {
		get_licenses(&handle, 1);
	}

	alpm_release(handle);

	return 0;
}