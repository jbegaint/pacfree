#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alpm.h>
#include <alpm_list.h>

#define ROOT "/"
#define PATH "/var/lib/pacman"

static int licenses_counter = 0;
static int pkgs_counter = 0;
static int free_counter = 0;

static int limit_output = 1;

typedef struct {
	const char *name;
	int count;
	alpm_list_t *pkgs;
} license_t;

char *OPEN_SOURCE_LICENSES[20] = {
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
	"MIT",
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

static int is_license_libre(const char* license)
{
	int i;
	for (i = 0; OPEN_SOURCE_LICENSES[i]; i++) {
		if (!strcmp(license, OPEN_SOURCE_LICENSES[i]))
			return 1;
	}
	return 0;
}

static void print_licenses_list(alpm_list_t *licenses)
{
	alpm_list_t *l;
	license_t *license;
	int i = 0;
	int sum = 0;

	for (l = licenses; l; l = l->next) {
		license = (license_t *) l->data;

		printf("+%-8s \n  %05.2f%% (%d/%d)\n", license->name,
			((double)license->count*100)/licenses_counter, license->count, licenses_counter);

		/* let's display only the 5 most used license */
		if (limit_output && (i == 4))
			break;

		sum += license->count;
		i++;
	}

	if (limit_output)
		printf("%-8s \n  %05.2f%%  (%d/%d)\n", "+other", 
			((double)(licenses_counter-sum)*100)/licenses_counter, licenses_counter-sum, licenses_counter);
}

static void free_licenses_list(alpm_list_t *list)
{
	alpm_list_t *l;

	/* free pkgs list fotr each license in list */
	for (l = list; l; l = l->next)
		alpm_list_free(((license_t *) l->data)->pkgs);

	/* free list and data */
	alpm_list_free_inner(list, free);
	alpm_list_free(list);
}

static license_t *get_license_in_list(alpm_list_t *licenses, char *name)
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

static void process_license(alpm_list_t **licenses, char *name, alpm_pkg_t *pkg)
{
	license_t *res;
	const char *pkgname;

	if ((res = get_license_in_list(*licenses, name))) {
		pkgname = alpm_pkg_get_name(pkg);

		res->count += 1;
		res->pkgs = alpm_list_add(res->pkgs, (void*) pkgname);
	} 
	else {
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

static void init_db(alpm_handle_t **handle, alpm_db_t **db)
{
	alpm_errno_t err;

	*handle = alpm_initialize(ROOT, PATH, &err);

	if (!*handle)
		die("Error: %s", alpm_strerror(err));

	*db = alpm_get_localdb(*handle);
}

static void usage(char *str)
{
	die("usage: %s [-a] [-l license/ list] [-v]\n", str);
}

static void init_licenses_list(alpm_db_t *db, alpm_list_t **licenses)
{
	alpm_list_t *i, *l;

	for (i = alpm_db_get_pkgcache(db); i; i = i->next) {
		alpm_pkg_t *pkg = i->data;

		l = alpm_pkg_get_licenses(pkg);

		/* iterate over multiple licenses */
		while(l) {
			process_license(licenses, (char*) l->data, pkg);
			licenses_counter++;
			l = l->next;
		}
		
		pkgs_counter++;
	}

	/* sort licenses by count */
	*licenses = sort_list(*licenses);
}

int main(int argc, char **argv)
{
	static alpm_handle_t *handle;
	static alpm_db_t *db;
	static alpm_list_t *licenses = NULL;

	init_db(&handle, &db);
	init_licenses_list(db, &licenses);

	print_licenses_list(licenses);
	printf("Free counter: %.2f%% (%d/%d)\n", (double)free_counter*100/pkgs_counter,
		free_counter, pkgs_counter);

	free_licenses_list(licenses);


	/*if ((argc == 2) && !strcmp("-v", argv[1])) {
		die("%s © 2013 jeanbroid\n", "pacfree");
	}
	else if ((argc == 2) && !strcmp("-a", argv[1])) {
		get_licenses_and_count(&handle, 0);
	}
	else if ((argc == 3) && !strcmp("-l", argv[1]) 
		&& !strcmp("list", argv[2])) {
 		get_licenses_list(&handle);
	}
	else if ((argc == 3) && !strcmp("-l", argv[1])) {
 		get_pkgs_by_license(&handle, argv[2]);
	}
	else if (argc != 1) {
		usage(argv[0]);
	}
	else {
		get_licenses_and_count(&handle, 1);
	}*/

	alpm_release(handle);

	return 0;
}