#ifndef UTIL_H
#define UTIL_H

// Shorthand for zend_parse_parameters().
#define STR_ARG(str) &str, &str##_len
#define RETURN_EMPTY_ARR() RETURN_ARR(u_create_table(0))

#include <stdint.h>
#include "php.h"

// Conversion from wide character string to multibyte string and vice versa.
char* u_wcstombs(const wchar_t *src);
wchar_t* u_mbstowcs(const char *src);

// Conversion of a wide character string to lower case.
wchar_t* u_wcstolower(wchar_t *str);                      // Modifies the passed string.
wchar_t* u_wcstolower_s(const wchar_t *str, size_t size); // Allocates a new string.

// Sets the out argument to the numeric value of the string
// if it only contains numeric characters, but remains unchanged if not.
// A non-zero return value indicates that the out value was set.
int u_is_number(const char *str, int *out);

// Calls a PHP function.
zval u_call_function(zval *object, const char *name, zval *params, uint32_t param_count);
// Allocates a hash table of a specific size.
HashTable *u_create_table(uint32_t size);

#endif // !UTIL_H
