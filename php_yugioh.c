#include <locale.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#define PHP_COMPILER_ID "VC14"
#include "php.h"
#include "ext/standard/info.h"
#include "php_yugioh.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "yugioh.h"
#include "replay_reader.h"
#include "sqlite3.h"
#include "util.h"

#define STR_ARG(str) &str, &str##_len
#define RETURN_EMPTY_ARR() RETURN_ARR(zend_hash_create(0))

static zend_class_entry *yugioh_class_entry = NULL;
static zend_class_entry *yugioh_card_class_entry = NULL;
static zend_class_entry *yugioh_replay_class_entry = NULL;
static zend_object_handlers yugioh_object_handlers;

static void replay_to_zval(zval **object, struct rr_replay *replay);

// Module entry
// {{{
zend_module_entry yugioh_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	PHP_YUGIOH_EXTNAME,
	NULL,
	PHP_MINIT(yugioh),
	PHP_MSHUTDOWN(yugioh),
	NULL,
	NULL,
	PHP_MINFO(yugioh),
	PHP_YUGIOH_VERSION,
	STANDARD_MODULE_PROPERTIES
};
// }}}

#ifdef COMPILE_DL_YUGIOH
ZEND_GET_MODULE(yugioh)
#endif

// Argument information
// {{{
ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_replay___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_replay_from_file, 0, 0, 1)
	ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_replay_read_file, 0, 0, 1)
	ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_replay_decode, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_card___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_db, 0, 0, 2)
	ZEND_ARG_INFO(0, lang)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_dbs, 0, 0, 1)
	ZEND_ARG_INFO(0, paths)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_db_remove, 0, 0, 1)
	ZEND_ARG_INFO(0, lang)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_match, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, lang)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_search, 0, 0, 3)
	ZEND_ARG_INFO(0, any)
	ZEND_ARG_INFO(0, inlang)
	ZEND_ARG_INFO(0, lang)
ZEND_END_ARG_INFO()
// }}}

// yugioh\replay - Class method entries
// {{{
static const zend_function_entry yugioh_replay_class_method_entry[] = {
	PHP_ME(yugioh_replay, __construct, arginfo_yugioh_replay___construct, ZEND_ACC_PUBLIC)
	PHP_ME(yugioh_replay, from_file, arginfo_yugioh_replay_from_file, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(yugioh_replay, read_file, arginfo_yugioh_replay_read_file, ZEND_ACC_PUBLIC)
	PHP_ME(yugioh_replay, decode, arginfo_yugioh_replay_decode, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
// }}}

// yugioh\card - Class method entries
// {{{
static const zend_function_entry yugioh_card_class_method_entry[] = {
	PHP_ME(yugioh_card, __construct, arginfo_yugioh_card___construct, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
// }}}

// yugioh - Class method entries
// {{{
static const zend_function_entry yugioh_class_method_entry[] = {
	PHP_ME(yugioh, __construct, arginfo_yugioh___construct, ZEND_ACC_PUBLIC)
	PHP_ME(yugioh, db, arginfo_yugioh_db, ZEND_ACC_PUBLIC)
	PHP_ME(yugioh, dbs, arginfo_yugioh_dbs, ZEND_ACC_PUBLIC)
	PHP_ME(yugioh, db_remove, arginfo_yugioh_db_remove, ZEND_ACC_PUBLIC)
	PHP_ME(yugioh, match, arginfo_yugioh_match, ZEND_ACC_PUBLIC)
	PHP_ME(yugioh, search, arginfo_yugioh_search, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
// }}}

// public function yugioh\replay::__construct(void) : void
// {{{
PHP_METHOD(yugioh_replay, __construct)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	zval *self = getThis();
	HashTable *players = zend_hash_create(0);

	zval players_zv;
	ZVAL_ARR(&players_zv, players);
	zend_update_property(yugioh_replay_class_entry, self, "players", sizeof("players") - 1, &players_zv);
}
// }}}

// public static function yugioh\replay::from_file(string $file) : yugioh\replay
// {{{
PHP_METHOD(yugioh_replay, from_file)
{
	char *file = NULL;
	size_t file_len;

	int argc = ZEND_NUM_ARGS();
	if (zend_parse_parameters(argc, "s", STR_ARG(file)) == FAILURE)
		RETURN_NULL();

	object_init_ex(return_value, yugioh_replay_class_entry);

	zval func_name, rv, argv;
	ZVAL_STRINGL(&argv, file, file_len);

	ZVAL_LSTRING(&func_name, "__construct");
	call_user_function(&Z_CE_P(return_value)->function_table, return_value, &func_name, &rv, 0, NULL);

	ZVAL_LSTRING(&func_name, "read_file");
	call_user_function(&Z_CE_P(return_value)->function_table, return_value, &func_name, &rv, 1, &argv);
}
// }}}

// public function yugioh\replay::read_file(string $file) : void
// {{{
PHP_METHOD(yugioh_replay, read_file)
{
	char *file = NULL;
	size_t file_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", STR_ARG(file)) == FAILURE)
		return;

	char file_path[MAXPATHLEN];
	VCWD_REALPATH(file, file_path);
	if (!file_path[0]) {
		php_error_docref(NULL, E_ERROR, "file does not exit");
		return;
	}

	struct rr_replay *replay = rr_read_replay(file_path);
	if (!replay)
		return;

	zval *self = getThis();
	replay_to_zval(&self, replay);
	rr_destroy_replay(replay);
}
// }}}

// public function yugioh\replay::decode(string $data) : void
// {{{
PHP_METHOD(yugioh_replay, decode)
{
	unsigned char *data = NULL;
	size_t data_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", STR_ARG(data)) == FAILURE)
		return;

	struct rr_replay *replay = rr_read_replay_a(data, data_len);
	if (!replay)
		return;

	zval *self = getThis();
	replay_to_zval(&self, replay);
	rr_destroy_replay(replay);
}

// public function yugioh\card::__construct(void) : void
// {{{
PHP_METHOD(yugioh_card, __construct)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	zval *self = getThis();
	HashTable *ids = zend_hash_create(0);

	zval ids_zv;
	ZVAL_ARR(&ids_zv, ids);
	zend_update_property(yugioh_card_class_entry, self, "ids", sizeof("ids") - 1, &ids_zv);
}
// }}}

// public function yugioh::__construct(void) : void
// {{{
PHP_METHOD(yugioh, __construct)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	zval *self = getThis();
	HashTable *databases = zend_hash_create(0);

	zval databases_zv;
	ZVAL_ARR(&databases_zv, databases);
	zend_update_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, &databases_zv);
}
// }}}

// public function yugioh::db(string $language, string $path) : void
// {{{
PHP_METHOD(yugioh, db)
{
	char *lang = NULL, *path = NULL;
	size_t lang_len, path_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", STR_ARG(lang), STR_ARG(path)) == FAILURE)
		return;

	zval *self = getThis(), *rv = NULL;
	zval *databases_zv = zend_read_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, 0, rv);
	HashTable *databases = Z_ARR_P(databases_zv);

	zval path_zv;
	ZVAL_STRING(&path_zv, path);
	zend_hash_str_add(databases, lang, lang_len, &path_zv);
}
// }}}

// public function yugioh::dbs(array $paths) : void
// {{{
PHP_METHOD(yugioh, dbs)
{
	HashTable *paths_ht = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "H", &paths_ht) == FAILURE)
		return;

	zval *self = getThis(), *rv = NULL;
	zval *databases_zv = zend_read_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, 0, rv);

	HashTable *databases = Z_ARR_P(databases_zv);
	HashPosition pos;

	for (zend_hash_internal_pointer_reset_ex(paths_ht, &pos);
		zend_hash_has_more_elements_ex(paths_ht, &pos) == SUCCESS;
		zend_hash_move_forward_ex(paths_ht, &pos))
	{
		zend_string *lang;
		if (zend_hash_get_current_key_ex(paths_ht, &lang, 0, &pos) ^ HASH_KEY_IS_STRING)
			continue;

		zval *path = zend_hash_get_current_data_ex(paths_ht, &pos);
		zend_hash_str_add(databases, lang->val, lang->len, path);
	}
}
// }}}

// public function yugioh::db_remove(string $language) : void
// {{{
PHP_METHOD(yugioh, db_remove)
{
	char *lang = NULL;
	size_t lang_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", STR_ARG(lang)) == FAILURE)
		return;

	zval *self = getThis(), *rv = NULL;

	zval *databases_zv = zend_read_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, 0, rv);
	HashTable *databases = Z_ARR_P(databases_zv);

	zval lang_zv;
	ZVAL_STRING(&lang_zv, lang);
	zend_hash_del_ind(databases, Z_STR_P(&lang_zv));
}
// }}}

// public function yugioh::match(string $name, string $inlang, number $count = 1) : array
// {{{
PHP_METHOD(yugioh, match)
{
	char *name = NULL, *lang = NULL;
	size_t name_len, lang_len;
	long count = 1l;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss|l", STR_ARG(name), STR_ARG(lang), &count) == FAILURE)
		RETURN_EMPTY_ARR();

	zval *self = getThis(), *rv = NULL;
	zval *databases_zv = zend_read_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, 0, rv);
	HashTable *databases = Z_ARR_P(databases_zv);

	zval lang_z;
	ZVAL_STRING(&lang_z, lang);
	zend_string *lang_zs = Z_STR(lang_z);
	if (!zend_hash_exists_ind(databases, lang_zs))
		RETURN_EMPTY_ARR();

	char lang_path[MAXPATHLEN];
	VCWD_REALPATH(Z_STR(*zend_hash_find(databases, lang_zs))->val, lang_path);

	int err = 0;
	size_t size = 0;
	wchar_t *name_wcs = u_mbstowcs(name);
	struct yugioh_entry *entries = yugioh_match(name_wcs, lang_path, &size, &err);
	free(name_wcs);

	if (err) {
		php_error_docref(NULL, E_ERROR, "sqlite3 error: %s (code: %i)", sqlite3_errstr(err), err);
		return;
	}
	if (!entries)
		RETURN_EMPTY_ARR();

	HashTable *result = zend_hash_create(count < 0 ? 0 : count);
	size_t i;
	for (i = 0llu; i < count && i < size; ++i) {
		struct yugioh_entry entry = entries[i];

		HashTable *entry_ht = zend_hash_create(3);
		char *entry_name = u_wcstombs(entry.name);

		zval name_zv, distance_zv, contains_zv;
		ZVAL_STRING(&name_zv, entry_name);
		ZVAL_DOUBLE(&distance_zv, entry.distance);
		ZVAL_BOOL(&contains_zv, entry.contains);

		zend_hash_str_add(entry_ht, "name", 4, &name_zv);
		zend_hash_str_add(entry_ht, "distance", 8, &distance_zv);
		zend_hash_str_add(entry_ht, "contains", 8, &contains_zv);

		zval entry_zv;
		ZVAL_ARR(&entry_zv, entry_ht);
		zend_hash_next_index_insert(result, &entry_zv);

		free(entry_name);
	}

	free(entries);
	RETURN_ARR(result);
}
// }}}

// public function yugioh::search(string $any, string $inlang, string $lang) : yugioh\card
// {{{
PHP_METHOD(yugioh, search)
{
	char *name, *inlang, *lang;
	size_t name_len, inlang_len, lang_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sss", STR_ARG(name), STR_ARG(inlang), STR_ARG(lang)) == FAILURE)
		RETURN_NULL();

	zval *self = getThis(), *rv;
	zval *databases_zv = zend_read_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, 0, rv);
	HashTable *databases = Z_ARR_P(databases_zv);

	zval lang_z;
	ZVAL_STRING(&lang_z, lang);
	zend_string *lang_zs = Z_STR(lang_z);
	if (!zend_hash_exists_ind(databases, lang_zs))
		RETURN_NULL();

	char lang_path[MAXPATHLEN];
	VCWD_REALPATH(Z_STR(*zend_hash_find(databases, lang_zs))->val, lang_path);

	int id = 0, err = 0;
	struct yugioh_card *card = NULL;
	if (u_is_number(name, &id))
		card = yugioh_search(id, lang_path, &err);
	else {
		zval inlang_z;
		ZVAL_STRING(&inlang_z, inlang);
		zend_string *inlang_zs = Z_STR(inlang_z);
		if (!zend_hash_exists_ind(databases, inlang_zs))
			RETURN_NULL();

		char inlang_path[MAXPATHLEN];
		VCWD_REALPATH(Z_STR(*zend_hash_find(databases, inlang_zs))->val, inlang_path);

		wchar_t *name_wcs = u_mbstowcs(name);
		card = yugioh_search_n(name_wcs, inlang_path, lang_path, &err);
		free(name_wcs);
	}

	if (err) {
		php_error_docref(NULL, E_ERROR, "sqlite3 error: %s (code: %i)", sqlite3_errstr(err), err);
		return;
	}
	if (!card)
		RETURN_NULL();

	object_init_ex(return_value, yugioh_card_class_entry);
	HashTable *ids = zend_hash_create(8);
	
	size_t i;
	for (i = 0; i < card->ids_size; ++i) {
		zval zv;
		ZVAL_LONG(&zv, card->ids[i]);
		zend_hash_next_index_insert(ids, &zv);
	}

	char *name_mbs = u_wcstombs(card->name);
	char *desc_mbs = u_wcstombs(card->desc);

	zval z_ids, z_name_mbs, z_desc_mbs, z_ot, z_setcode, z_type, z_atk, z_def, z_level, z_race, z_attribute, z_category;
	ZVAL_ARR(&z_ids, ids);
	ZVAL_STRING(&z_name_mbs, name_mbs);
	ZVAL_STRING(&z_desc_mbs, desc_mbs);
	ZVAL_LONG(&z_ot, card->ot);
	ZVAL_LONG(&z_setcode, card->setcode);
	ZVAL_LONG(&z_type, card->type);
	ZVAL_LONG(&z_atk, card->atk);
	ZVAL_LONG(&z_def, card->def);
	ZVAL_LONG(&z_level, card->level);
	ZVAL_LONG(&z_race, card->race);
	ZVAL_LONG(&z_attribute, card->attribute);
	ZVAL_LONG(&z_category, card->category);

	if (card->original_id) {
		zval z_original_id;
		ZVAL_LONG(&z_original_id, *card->original_id);
		zend_update_property(yugioh_card_class_entry, return_value, "original_id", sizeof("original_id") - 1, &z_original_id);
	}
	zend_update_property(yugioh_card_class_entry, return_value, "ids", sizeof("ids") - 1, &z_ids);
	zend_update_property(yugioh_card_class_entry, return_value, "name", sizeof("name") - 1, &z_name_mbs);
	zend_update_property(yugioh_card_class_entry, return_value, "desc", sizeof("desc") - 1, &z_desc_mbs);
	zend_update_property(yugioh_card_class_entry, return_value, "ot", sizeof("ot") - 1, &z_ot);
	zend_update_property(yugioh_card_class_entry, return_value, "setcode", sizeof("setcode") - 1, &z_setcode);
	zend_update_property(yugioh_card_class_entry, return_value, "type", sizeof("type") - 1, &z_type);
	zend_update_property(yugioh_card_class_entry, return_value, "atk", sizeof("atk") - 1, &z_atk);
	zend_update_property(yugioh_card_class_entry, return_value, "def", sizeof("def") - 1, &z_def);
	zend_update_property(yugioh_card_class_entry, return_value, "level", sizeof("level") - 1, &z_level);
	zend_update_property(yugioh_card_class_entry, return_value, "race", sizeof("race") - 1, &z_race);
	zend_update_property(yugioh_card_class_entry, return_value, "attribute", sizeof("attribute") - 1, &z_attribute);
	zend_update_property(yugioh_card_class_entry, return_value, "category", sizeof("category") - 1, &z_category);

	free(name_mbs);
	free(desc_mbs);
	yugioh_destroy_card(card);
}
// }}}

static void replay_to_zval(zval **object, struct rr_replay *replay)
{
	if (!object || !*object)
		return;

	zval *zend_value = *object;

	zval life_points_zv, hand_count_zv, draw_count_zv;
	ZVAL_LONG(&life_points_zv, replay->life_points);
	ZVAL_LONG(&hand_count_zv, replay->hand_count);
	ZVAL_LONG(&draw_count_zv, replay->draw_count);
	zend_update_property(yugioh_replay_class_entry, zend_value, "life_points", sizeof("life_points") - 1, &life_points_zv);
	zend_update_property(yugioh_replay_class_entry, zend_value, "hand_count", sizeof("hand_count") - 1, &hand_count_zv);
	zend_update_property(yugioh_replay_class_entry, zend_value, "draw_count", sizeof("draw_count") - 1, &draw_count_zv);

	zval *rv = NULL;
	zval *players_zv = zend_read_property(yugioh_replay_class_entry, zend_value, "players", sizeof("players") - 1, 0, rv);
	HashTable *players = Z_ARR_P(players_zv);

	int32_t i;
	size_t j;
	for (i = 0; i < replay->player_count; ++i) {
		char *player = replay->players[i];
		struct rr_deck_info deck = replay->decks[i];

		if (deck.size_main == 0) {
			zval null_zv;
			ZVAL_NULL(&null_zv);
			zend_hash_next_index_insert(players, &null_zv);
			continue;
		}

		HashTable *main_deck_ht = zend_hash_create((uint32_t)deck.size_main);
		for (j = 0; j < deck.size_main; ++j) {
			zval card_zv;
			ZVAL_LONG(&card_zv, deck.main_deck[j]);
			zend_hash_next_index_insert(main_deck_ht, &card_zv);
		}

		HashTable *extra_deck_ht = zend_hash_create((uint32_t)deck.size_extra);
		for (j = 0; j < deck.size_extra; ++j) {
			zval card_zv;
			ZVAL_LONG(&card_zv, deck.extra_deck[j]);
			zend_hash_next_index_insert(extra_deck_ht, &card_zv);
		}

		HashTable *deck_ht = zend_hash_create(2);
		zval player_zv, main_deck_zv, extra_deck_zv;
		ZVAL_STRING(&player_zv, player);
		ZVAL_ARR(&main_deck_zv, main_deck_ht);
		ZVAL_ARR(&extra_deck_zv, extra_deck_ht);
		zend_hash_str_add(deck_ht, "name", 4, &player_zv);
		zend_hash_str_add(deck_ht, "main_deck", 9, &main_deck_zv);
		zend_hash_str_add(deck_ht, "extra_deck", 10, &extra_deck_zv);

		zval deck_zv;
		ZVAL_ARR(&deck_zv, deck_ht);
		zend_hash_next_index_insert(players, &deck_zv);
	}
}

static zend_object *yugioh_create_object(zend_class_entry *ce)
{
	zend_object *zo = ecalloc(1, sizeof(zend_object) + zend_object_properties_size(ce));

	zend_object_std_init(zo, ce);
	object_properties_init(zo, ce);

	zo->handlers = &yugioh_object_handlers;
	return zo;
}

// PHP module information
// {{{
PHP_MINFO_FUNCTION(yugioh)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Yugioh Support", "enabled");
	php_info_print_table_row(2, "Yugioh Version", PHP_YUGIOH_VERSION);
	php_info_print_table_end();
	DISPLAY_INI_ENTRIES();
}
// }}}

// PHP module initialization
// {{{
PHP_MINIT_FUNCTION(yugioh)
{
	// class yugioh\replay
	// {{{
	zend_class_entry replay_ce;
	INIT_CLASS_ENTRY(replay_ce, "yugioh\\replay", yugioh_replay_class_method_entry);
	yugioh_replay_class_entry = zend_register_internal_class(&replay_ce);
	yugioh_replay_class_entry->create_object = yugioh_create_object;

	zend_declare_property_long(yugioh_replay_class_entry, "life_points", sizeof("life_points") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_replay_class_entry, "hand_count", sizeof("hand_count") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_replay_class_entry, "draw_count", sizeof("draw_count") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_null(yugioh_replay_class_entry, "players", sizeof("players") - 1, ZEND_ACC_PUBLIC);
	// }}}

	// class yugioh\card
	// {{{
	zend_class_entry card_ce;
	INIT_CLASS_ENTRY(card_ce, "yugioh\\card", yugioh_card_class_method_entry);
	yugioh_card_class_entry = zend_register_internal_class(&card_ce);
	yugioh_card_class_entry->create_object = yugioh_create_object;

	zend_declare_property_long(yugioh_card_class_entry, "original_id", sizeof("original_id") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_null(yugioh_card_class_entry, "ids", sizeof("ids") - 1, ZEND_ACC_PUBLIC);
	zend_declare_property_string(yugioh_card_class_entry, "name", sizeof("name") - 1, "", ZEND_ACC_PUBLIC);
	zend_declare_property_string(yugioh_card_class_entry, "desc", sizeof("desc") - 1, "", ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "ot", sizeof("ot") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "setcode", sizeof("setcode") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "type", sizeof("type") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "atk", sizeof("atk") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "def", sizeof("def") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "level", sizeof("level") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "race", sizeof("race") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "attribute", sizeof("attribute") - 1, 0, ZEND_ACC_PUBLIC);
	zend_declare_property_long(yugioh_card_class_entry, "category", sizeof("category") - 1, 0, ZEND_ACC_PUBLIC);
	// }}}

	// class yugioh
	// {{{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "yugioh", yugioh_class_method_entry);
	yugioh_class_entry = zend_register_internal_class(&ce);
	yugioh_class_entry->create_object = yugioh_create_object;

	zend_declare_property_null(yugioh_class_entry, "dbs", sizeof("dbs") - 1, ZEND_ACC_PUBLIC);
	// }}}

	memcpy(&yugioh_object_handlers, zend_get_std_object_handlers(), sizeof(yugioh_object_handlers));
	yugioh_object_handlers.free_obj = zend_object_std_dtor;
	yugioh_object_handlers.dtor_obj = zend_objects_destroy_object;
	
	return SUCCESS;
}
// }}}

// PHP module shutdown
// {{{
PHP_MSHUTDOWN_FUNCTION(yugioh)
{
	yugioh_class_entry = NULL;
	yugioh_card_class_entry = NULL;
	return SUCCESS;
}
// }}}
