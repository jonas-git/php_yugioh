#include <locale.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "yugioh.h"
#include "conv.h"
#include "dice.h"

#define CHECK_ALLOC(ptr) if (!ptr) { exit(EXIT_FAILURE); }

struct yugioh_entry_node;
struct yugioh_entry_node
{
	struct yugioh_entry *data;
	struct yugioh_entry_node *next;
};

struct yugioh_id_node;
struct yugioh_id_node
{
	int32_t id;
	struct yugioh_id_node *next;
};

typedef struct yugioh_card yugioh_card;
typedef struct yugioh_entry yugioh_entry;
typedef struct yugioh_entry_node yugioh_entry_node;
typedef struct yugioh_id_node yugioh_id_node;

inline yugioh_entry_node* new_entry_node()
{
	yugioh_entry_node *mem = (yugioh_entry_node *)calloc(1, sizeof(yugioh_entry_node));
	CHECK_ALLOC(mem);
	mem->data = NULL;
	mem->next = NULL;
	return mem;
}

inline yugioh_id_node* new_id_node()
{
	yugioh_id_node *mem = (yugioh_id_node *)calloc(1, sizeof(yugioh_id_node));
	CHECK_ALLOC(mem);
	mem->id = 0u;
	mem->next = NULL;
	return mem;
}

bool sqlite3_check(int err_code, int *out_err, sqlite3 *db, sqlite3_stmt *stmt)
{
	if (err_code ^ SQLITE_OK) {
		if (out_err) { *out_err = err_code; }
		if (stmt) sqlite3_finalize(stmt);
		if (db) sqlite3_close(db);
		return false;
	}
	return true;
}

int sort_predicate(const void* p1, const void* p2)
{
	const yugioh_entry a = *(yugioh_entry *)p1;
	const yugioh_entry b = *(yugioh_entry *)p2;

	if (a.distance == b.distance) {
		if (a.contains && b.contains) {
			return a.name < b.name ? -1 : 1;
		}
		return a.contains ? -1 : 1;
	}
	return a.distance > b.distance ? -1 : 1;
}

yugioh_entry* yugioh_match(const wchar_t *name, const char *path, size_t *out_size, int *out_err)
{
	setlocale(LC_ALL, "en_US.UTF-8");
	wchar_t *name_lower = wcs_to_lower_s(name, wcslen(name));

	sqlite3 *db;
	sqlite3_stmt *stmt;
	int err_code = 0;

	err_code = sqlite3_open(path, &db);
	if (!sqlite3_check(err_code, out_err, db, NULL)) { return NULL; }

	sqlite3_prepare(db, "SELECT DISTINCT name FROM texts", -1, &stmt, 0);
	err_code = sqlite3_step(stmt);
	if (err_code ^ SQLITE_ROW) {
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return NULL;
	}

	size_t list_size = 0;
	yugioh_entry_node *entry_list = new_entry_node();
	yugioh_entry_node *current_node = entry_list;

	while (1) {
		++list_size;

		wchar_t *cdb_name = mbs_to_wcs(sqlite3_column_text(stmt, 0));
		wchar_t *cdb_name_lower = wcs_to_lower_s(cdb_name, wcslen(cdb_name) + 1);

		current_node->data = (yugioh_entry *)calloc(1, sizeof(yugioh_entry));
		CHECK_ALLOC(current_node->data);
		current_node->data->name = cdb_name;
		current_node->data->distance = dice_coefficient(name_lower, cdb_name_lower);
		current_node->data->contains = !!wcsstr(cdb_name_lower, name_lower);

		err_code = sqlite3_step(stmt);
		if (err_code ^ SQLITE_ROW) {
			break;
		}

		current_node = current_node->next = new_entry_node();
	}

	if (out_size) {
		*out_size = list_size;
	}
	yugioh_entry *entries = (yugioh_entry *)calloc(list_size, sizeof(yugioh_entry));
	CHECK_ALLOC(entries);
	yugioh_entry *current_entry = entries;

	current_node = entry_list;
	while (current_node) {
		*current_entry++ = *current_node->data;
	 	yugioh_entry_node *next = current_node->next;
	 	free(current_node);
	 	current_node = next;
	}

	qsort(entries, list_size, sizeof(yugioh_entry), sort_predicate);

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return entries;
}

yugioh_card* to_card(sqlite3_stmt *stmt)
{
	if (sqlite3_step(stmt) ^ SQLITE_ROW) {
		return NULL;
	}

	yugioh_card *card = (yugioh_card *)calloc(1, sizeof(yugioh_card));
	CHECK_ALLOC(card);

	card->ids = NULL;
	card->original_id = NULL;
	card->name = mbs_to_wcs(sqlite3_column_text(stmt, 1));
	card->desc = mbs_to_wcs(sqlite3_column_text(stmt, 2));
	card->ot = sqlite3_column_int(stmt, 3);
	card->setcode = sqlite3_column_int(stmt, 5);
	card->type = sqlite3_column_int(stmt, 6);
	card->atk = sqlite3_column_int(stmt, 7);
	card->def = sqlite3_column_int(stmt, 8);
	card->level = sqlite3_column_int(stmt, 9);
	card->race = sqlite3_column_int(stmt, 10);
	card->attribute = sqlite3_column_int(stmt, 11);
	card->category = sqlite3_column_int(stmt, 12);

	size_t list_size = 0;
	yugioh_id_node *id_list = new_id_node();
	yugioh_id_node *current_node = id_list;

	int err_code = 0;
	int32_t alias = 0;
	while (1) {
		++list_size;
		current_node->id = sqlite3_column_int(stmt, 0);

		err_code = sqlite3_step(stmt);
		if (err_code ^ SQLITE_ROW) {
			break;
		}

		current_node = current_node->next = new_id_node();
		alias = sqlite3_column_int(stmt, 4);
	}

	int32_t *ids = (int32_t *)calloc(list_size, sizeof(int32_t));
	CHECK_ALLOC(ids);
	int32_t *current_id = ids;

	current_node = id_list;
	while (current_node) {
		*current_id++ = current_node->id;
	 	yugioh_id_node *next = current_node->next;
	 	free(current_node);
	 	current_node = next;
	}

	card->ids = ids;
	card->ids_size = list_size;

	if (alias) {
		current_id = ids;
		while (current_id) {
			if (*current_id == alias) {
				card->original_id = current_id;
				break;
			}
			++current_id;
		}
	}

	if (!card->original_id) {
		card->original_id = ids;
	}

	return card;
}

yugioh_card* yugioh_search(int32_t id, const char *path, int *out_err)
{
	sqlite3 *db;
	sqlite3_stmt *stmt;
	int err_code = 0;

	err_code = sqlite3_open(path, &db);
	if (!sqlite3_check(err_code, out_err, db, NULL)) { return NULL; }

	sqlite3_prepare(db, "SELECT t.id, t.name, t.desc, d.ot, d.alias, d.setcode,"
		" d.type, d.atk, d.def, d.level, d.race, d.attribute, d.category"
		" FROM texts AS t JOIN datas AS d ON t.id = d.id WHERE name IN"
		" (SELECT name FROM texts WHERE id=?) COLLATE NOCASE", -1, &stmt, NULL);
	err_code = sqlite3_bind_int(stmt, 1, id);
	if (!sqlite3_check(err_code, out_err, db, stmt)) { return NULL; }

	yugioh_card *card = to_card(stmt);

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return card;
}

struct yugioh_card* yugioh_search_n(const wchar_t *name, const char *in_path, const char *path, int *out_err)
{
	sqlite3 *db;
	sqlite3_stmt *stmt;
	int err_code = 0;

	err_code = sqlite3_open(in_path, &db);
	if (!sqlite3_check(err_code, out_err, db, NULL)) { return NULL; }

	const wchar_t *name_wcs;
	if (strcmp(in_path, path) == 0) {
		name_wcs = name;
	}
	else {
		char *name_mbs = wcs_to_mbs(name);

		sqlite3_prepare(db, "SELECT id FROM texts WHERE name LIKE ? COLLATE NOCASE LIMIT 1", -1, &stmt, NULL);
		err_code = sqlite3_bind_text(stmt, 1, name_mbs, -1, NULL);
		if (!sqlite3_check(err_code, out_err, db, stmt)) { return NULL; }

		err_code = sqlite3_step(stmt);
		if (err_code ^ SQLITE_ROW) {
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			return NULL;
		}
		int id = sqlite3_column_int(stmt, 0);

		sqlite3_finalize(stmt);
		sqlite3_close(db);

		err_code = sqlite3_open(path, &db);
		if (!sqlite3_check(err_code, out_err, db, stmt)) { return NULL; }

		sqlite3_prepare(db, "SELECT name FROM texts WHERE id=? LIMIT 1", -1, &stmt, NULL);
		err_code = sqlite3_bind_int(stmt, 1, id);
		if (!sqlite3_check(err_code, out_err, db, stmt)) { return NULL; }

		err_code = sqlite3_step(stmt);
		if (err_code ^ SQLITE_ROW) {
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			return NULL;
		}
		name_wcs = mbs_to_wcs(sqlite3_column_text(stmt, 0));

		sqlite3_finalize(stmt);
	}
	
	sqlite3_prepare(db, "SELECT t.id, t.name, t.desc, d.ot, d.alias, d.setcode,"
		" d.type, d.atk, d.def, d.level, d.race, d.attribute, d.category"
		" FROM texts AS t JOIN datas AS d ON t.id = d.id WHERE t.name=? COLLATE NOCASE", -1, &stmt, NULL);
	err_code = sqlite3_bind_text(stmt, 1, wcs_to_mbs(name_wcs) /*name_wcs*/, -1, NULL);
	if (!sqlite3_check(err_code, out_err, db, stmt)) { return NULL; }

	yugioh_card *card = to_card(stmt);

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return card;
}
