#ifndef PHP_YUGIOH_H
#define PHP_YUGIOH_H
#define PHP_YUGIOH_VERSION "1.0.0"
#define PHP_YUGIOH_EXTNAME "yugioh"

extern zend_module_entry yugioh_module_entry;
#define phpext_yugioh_ptr &yugioh_module_entry

PHP_MINFO_FUNCTION(yugioh);
PHP_MINIT_FUNCTION(yugioh);
PHP_MSHUTDOWN_FUNCTION(yugioh);

PHP_METHOD(yugioh_card, __construct);

PHP_METHOD(yugioh, __construct);
PHP_METHOD(yugioh, db);
PHP_METHOD(yugioh, dbs);
PHP_METHOD(yugioh, db_remove);
PHP_METHOD(yugioh, match);
PHP_METHOD(yugioh, search);

#endif // !PHP_YUGIOH_H
