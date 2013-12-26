#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alpm.h>
#include <alpm_list.h>

typedef struct {
	char *name;
	int count;
} license_t;

void print_license_list(alpm_list_t *licenses, int counter)
{
	alpm_list_t *l;
	license_t *license;

	for (l = licenses; l; l = l->next) {
		license = (license_t *) l->data;
		printf("%3d (%05.2f%%) %s \n", license->count, 
			(double)license->count*100/counter, license->name);
	}
}

void free_list(alpm_list_t *list)
{
	/* free list and data */
	alpm_list_free_inner(list, free);
	alpm_list_free(list);
}

void *find_license_in_list(alpm_list_t *licenses, char *name)
{
	alpm_list_t *l;
	license_t *license;

	for (l = licenses; l; l = l->next) {
		license = (license_t *) l->data;

		if (strcmp(name, license->name) == 0)
			return license;
	}

	return NULL;
}

int cmp_count(const void *data1, const void *data2)
{
	/* cast date and return diff */
	return (((license_t *) data2)->count - ((license_t *) data1)->count);
}

void process_license(alpm_list_t **licenses, char *name)
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

alpm_list_t *sort_list(alpm_list_t *list)
{
	alpm_list_t *list_copy = NULL;
	alpm_list_t *l;

	for (l = list; l; l = l->next)
		list_copy = alpm_list_add_sorted(list_copy, l->data, cmp_count);

	/* we get rid of the list, but we keep the data */
	alpm_list_free(list);

	return list_copy;
}

int main()
{
	const char *root = "/";
	const char *dbpath = "/var/lib/pacman";

	static alpm_handle_t *handle;
	static alpm_db_t *db;

	alpm_list_t *licenses = NULL;
	alpm_list_t *i, *l;

	alpm_errno_t err;
	int counter = 0;

	handle = alpm_initialize(root, dbpath, &err);

	if (!handle)
		return 1;

	db = alpm_get_localdb(handle);

	for (i = alpm_db_get_pkgcache(db); i; i = alpm_list_next(i)) {
		alpm_pkg_t *pkg = i->data;

		for (l = alpm_pkg_get_licenses(pkg); l; l = l->next)
			process_license(&licenses, (char*) l->data);

		counter++;
	}

	licenses = sort_list(licenses);
	print_license_list(licenses, counter);

	free_list(licenses);
	alpm_release(handle);

	return 0;
}