#include <locale.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "php_yugioh.h"
#include "yugioh.h"
#include "conv.h"

#ifndef PHP_FE_END
#define PHP_FE_END { NULL, NULL, NULL }
#endif
#define STR_ARG(str) &str, &str##_len
#define RETURN_EMPTY_ARR() RETURN_ARR(allocate_hash_table(0, ZVAL_PTR_DTOR, 0))

zend_class_entry *yugioh_class_entry = NULL;
zend_class_entry *yugioh_card_class_entry = NULL;
static zend_object_handlers yugioh_object_handlers;

static zend_object* yugioh_create_object(zend_class_entry *ce);
static HashTable *allocate_hash_table(uint32_t nSize, dtor_func_t pDestructor, zend_bool persistent ZEND_FILE_LINE_DC);
static int cstoi(const char *s);

/* module entry */
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
	NULL,
	PHP_YUGIOH_VERSION,
	STANDARD_MODULE_PROPERTIES
};
// }}}

#ifdef COMPILE_DL_YUGIOH
ZEND_GET_MODULE(yugioh)
#endif

/* argument information */
// {{{
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

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_match, 0, 0, 3)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, lang)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_yugioh_search, 0, 0, 3)
	ZEND_ARG_INFO(0, any)
	ZEND_ARG_INFO(0, inlang)
	ZEND_ARG_INFO(0, lang)
ZEND_END_ARG_INFO()
// }}}

/* yugioh card class method entries */
// {{{
static const zend_function_entry yugioh_card_class_method_entry[] = {
	PHP_ME(yugioh_card, __construct, arginfo_yugioh_card___construct, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
// }}}

/* yugioh class method entries */
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

/* public function yugioh\card::__construct(void) : void */
// {{{
PHP_METHOD(yugioh_card, __construct)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	zval *self = getThis();
	HashTable *ids = allocate_hash_table(0, ZVAL_PTR_DTOR, 0);

	zval ids_zv;
	ZVAL_ARR(&ids_zv, ids);
	zend_update_property(yugioh_card_class_entry, self, "ids", sizeof("ids") - 1, &ids_zv);
}
// }}}

/* public function yugioh::__construct(void) : void */
// {{{
PHP_METHOD(yugioh, __construct)
{
	if (zend_parse_parameters_none() == FAILURE)
		return;

	zval *self = getThis();
	HashTable *databases = allocate_hash_table(0, ZVAL_PTR_DTOR, 0);

	zval databases_zv;
	ZVAL_ARR(&databases_zv, databases);
	zend_update_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, &databases_zv);
}
// }}}

/* public function yugioh::db(string $language, string $path) : void */
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

/* public function yugioh::dbs(array $paths) : void */
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

/* public function yugioh::db_remove(string $language) : void */
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

/* public function yugioh::match(string $name, string $inlang, number $count = 1) : array */
// {{{
PHP_METHOD(yugioh, match)
{
	char *name = NULL, *lang = NULL;
	size_t name_len, lang_len;
	long count = 1l;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss|l", STR_ARG(name), STR_ARG(lang), &count) == FAILURE) {
		RETURN_EMPTY_ARR();
	}

	zval *self = getThis(), *rv = NULL;
	zval *databases_zv = zend_read_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, 0, rv);
	HashTable *databases = Z_ARR_P(databases_zv);

	zval lang_z;
	ZVAL_STRING(&lang_z, lang);
	zend_string *lang_zs = Z_STR(lang_z);
	if (!zend_hash_exists_ind(databases, lang_zs)) {
		RETURN_EMPTY_ARR();
	}

	char lang_path[MAXPATHLEN];
	VCWD_REALPATH(Z_STR(*zend_hash_find(databases, lang_zs))->val, lang_path);

	int err = 0;
	size_t size = 0;
	wchar_t *name_wcs = mbs_to_wcs(name);
	struct yugioh_entry *entries = yugioh_match(name_wcs, lang_path, &size, &err);

	if (err) {
		php_error_docref(NULL, E_ERROR, "sqlite3 error: %s (code: %i)", sqlite3_errstr(err), err);
		RETURN_FALSE;
	}
	if (!entries) {
		RETURN_EMPTY_ARR();
	}

	HashTable *result = allocate_hash_table(count < 0 ? 0 : count, ZVAL_PTR_DTOR, 0);
	size_t i;
	for (i = 0llu; i < count && i < size; ++i) {
		struct yugioh_entry entry = entries[i];

		HashTable *entry_ht = allocate_hash_table(3, ZVAL_PTR_DTOR, 0);
		char *entry_name = wcs_to_mbs(entry.name);

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
	}

	free(entries);
	RETURN_ARR(result);
}
// }}}

/* public function search(string $any, string $inlang, string $lang) */
// {{{
PHP_METHOD(yugioh, search)
{
	char *name, *inlang, *lang;
	size_t name_len, inlang_len, lang_len;

	uint32_t argc = ZEND_NUM_ARGS();
	if (zend_parse_parameters(argc, "sss", STR_ARG(name), STR_ARG(inlang), STR_ARG(lang)) == FAILURE) {
		RETURN_FALSE;
	}

	zval *self = getThis(), *rv;
	zval *databases_zv = zend_read_property(yugioh_class_entry, self, "dbs", sizeof("dbs") - 1, 0, rv);
	HashTable *databases = Z_ARR_P(databases_zv);

	zval inlang_z, lang_z;
	ZVAL_STRING(&inlang_z, inlang);
	ZVAL_STRING(&lang_z, lang);
	zend_string *inlang_zs = Z_STR(inlang_z), *lang_zs = Z_STR(lang_z);
	if (!zend_hash_exists_ind(databases, inlang_zs) || !zend_hash_exists_ind(databases, lang_zs)) {
		RETURN_FALSE;
	}

	char inlang_path[MAXPATHLEN], lang_path[MAXPATHLEN];
	VCWD_REALPATH(Z_STR(*zend_hash_find(databases, inlang_zs))->val, inlang_path);
	VCWD_REALPATH(Z_STR(*zend_hash_find(databases, lang_zs))->val, lang_path);

	int id = cstoi(name), err = 0;
	struct yugioh_card *card = id
		? yugioh_search(id, lang_path, &err)
		: yugioh_search_n(mbs_to_wcs(name), inlang_path, lang_path, &err);

	if (err) {
		php_error_docref(NULL, E_ERROR, "sqlite3 error: %s (code: %i)", sqlite3_errstr(err), err);
		RETURN_FALSE;
	}
	if (!card) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, yugioh_card_class_entry);
	HashTable *ids = allocate_hash_table(8, ZVAL_PTR_DTOR, 0);
	
	size_t i;
	for (i = 0; i < card->ids_size; ++i) {
		zval zv;
		ZVAL_LONG(&zv, card->ids[i]);
		zend_hash_next_index_insert(ids, &zv);
	}

	char *name_mbs = wcs_to_mbs(card->name);
	char *desc_mbs = wcs_to_mbs(card->desc);

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

	// deallocate card object's memory.
}
// }}}

// {{{
static HashTable *allocate_hash_table(uint32_t nSize, dtor_func_t pDestructor, zend_bool persistent ZEND_FILE_LINE_DC)
{
	HashTable *ht;
	ALLOC_HASHTABLE(ht);
	zend_hash_init(ht, nSize, NULL, pDestructor, persistent);
	return ht;
}
// }}}

// {{{
static int cstoi(const char *s)
{
	int result = 0;
	for (; *s != '\0'; ++s)
	{
		int code = (int)*s;
		if (code < 48 || code > 57)
			return 0;
		result *= 10;
		result += code - 48;
	}
	return result;
}
// }}}

// {{{
static zend_object* yugioh_create_object(zend_class_entry *ce)
{
	zend_object *zo;
	zo = (zend_object *)(
		ecalloc(1, sizeof(zend_object) + zend_object_properties_size(ce)));

	zend_object_std_init(zo, ce);
	object_properties_init(zo, ce);

	zo->handlers = &yugioh_object_handlers;
	return zo;
}
// }}}

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(yugioh)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Yugioh Support", "enabled");
	php_info_print_table_row(2, "Yugioh Version", PHP_YUGIOH_VERSION);
	php_info_print_table_end();
	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* PHP_MINIT_FUNCTION */
// {{{
PHP_MINIT_FUNCTION(yugioh)
{
	/* class yugioh\card */
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

	/* class yugioh */
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

/* PHP_MSHUTDOWN_FUNCTION */
// {{{
PHP_MSHUTDOWN_FUNCTION(yugioh)
{
	yugioh_class_entry = NULL;
	yugioh_card_class_entry = NULL;
	return SUCCESS;
}
// }}}
