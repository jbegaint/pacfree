#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alpm.h>
#include <alpm_list.h>

typedef struct {
	char *name;
	int count;
} license_t;

void print_license_list(alpm_list_t *licenses)
{
	alpm_list_t *l;
	license_t *license;

	for (l = licenses; l; l = l->next) {
		license = (license_t *) l->data;
		printf("%s %d\n", license->name, license->count);
	}
}

void free_list(alpm_list_t *list)
{
	alpm_list_free_inner(list, free);
	alpm_list_free(list);
}

void *license_in_list(alpm_list_t *licenses, char *name)
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
	license_t *l1, *l2;
	l1 = (license_t *) data1;
	l2 = (license_t *) data2;

	return (l2->count - l1->count);
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
	void *res;
	int counter = 0;

	handle = alpm_initialize(root, dbpath, &err);

	if (!handle)
		return 1;

	db = alpm_get_localdb(handle);

	for (i = alpm_db_get_pkgcache(db); i; i = alpm_list_next(i)) {
		alpm_pkg_t *pkg = i->data;

		for (l = alpm_pkg_get_licenses(pkg); l; l = l->next) {

			if (!(res = license_in_list(licenses, (char *) l->data))) {
				license_t *license = calloc(1, sizeof(license_t));
				license->name = l->data;
				license->count = 1;

				licenses = alpm_list_add(licenses, license);
			} else {
				((license_t *) res)->count += 1;
			}
		}
		counter++;
	}

	licenses = sort_list(licenses);

	print_license_list(licenses);

	free_list(licenses);
	alpm_release(handle);

	return 0;
}
