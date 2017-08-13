#ifndef PHP_YUGIOH_H
#define PHP_YUGIOH_H

#define PHP_YUGIOH_VERSION "1.0.0"
#define PHP_YUGIOH_EXTNAME "yugioh"

/* macros for creating a ZVAL from a string literal
	without a call to strlen(), like in ZVAL_STRING().  */
#define ZVAL_LSTRING(z, s) do { \
		const char *_s = (s); \
		ZVAL_STRINGL(z, _s, sizeof(s) - 1); \
	} while (0)
#define ZVAL_PLSTRING(z, s) do { \
		const char *_s = (s); \
		ZVAL_PSTRINGL(z, _s, sizeof(s) - 1); \
	} while (0)

static inline
HashTable *zend_hash_create(uint32_t size)
{
	HashTable *ht;
	ALLOC_HASHTABLE(ht);
	zend_hash_init(ht, size, NULL, ZVAL_PTR_DTOR, 0);
	return ht;
}

PHP_METHOD(yugioh_replay, __construct);
PHP_METHOD(yugioh_replay, from_data);
PHP_METHOD(yugioh_replay, read_file);
PHP_METHOD(yugioh_replay, read_data);
PHP_METHOD(yugioh_replay, is_valid);

PHP_METHOD(yugioh_card, __construct);

PHP_METHOD(yugioh, __construct);
PHP_METHOD(yugioh, db);
PHP_METHOD(yugioh, dbs);
PHP_METHOD(yugioh, db_remove);
PHP_METHOD(yugioh, match);
PHP_METHOD(yugioh, search);

PHP_MINFO_FUNCTION(yugioh);
PHP_MINIT_FUNCTION(yugioh);
PHP_MSHUTDOWN_FUNCTION(yugioh);

#endif // !PHP_YUGIOH_H
