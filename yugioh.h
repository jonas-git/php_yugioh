#ifndef YUGIOH_H
#define YUGIOH_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

struct yugioh_entry
{
	wchar_t *name;
	double distance;
	int contains;
};

struct yugioh_card
{
	int32_t *original_id;
	int32_t *ids;
	size_t ids_size;
	wchar_t *name;
	wchar_t *desc;
	int32_t ot;
	int32_t setcode;
	int32_t type;
	int32_t atk;
	int32_t def;
	int32_t level;
	int32_t race;
	int32_t attribute;
	int32_t category;
};

struct yugioh_entry* yugioh_match(const wchar_t *name, const char *path, size_t *out_size, int *out_err);
struct yugioh_card* yugioh_search(int32_t id, const char *path, int *out_err);
struct yugioh_card* yugioh_search_n(const wchar_t *name, const char *in_path, const char *path, int *out_err);
void yugioh_destroy_card(struct yugioh_card *card);

#endif // !TEST_H
