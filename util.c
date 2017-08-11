#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "php.h"
#include "util.h"

char *u_wcstombs(const wchar_t *src) {
	size_t size = 0;
	const wchar_t *current = src;
	
	do {
		if (*current < 0x80)
			size += 1;
		else if (*current < 0x800)
			size += 2;
		else if (*current - 0xd800u < 0x800)
			return NULL;
		else if (*current < 0x10000)
			size += 3;
		else if (*current < 0x110000)
			size += 4;
		else
			return NULL;
	}
	while (*current++ != '\0');

	char *dest = calloc(size, sizeof(char)), *wcs = dest;
	if (!dest)
		exit(EXIT_FAILURE);

	do {
		wchar_t c = *src;
		if (c < 0x80)
			*wcs++ = c;
		else if (c < 0x800) {
			*wcs++ = 192 + c / (64);
			*wcs++ = 128 + c % 64;
		}
		else if (c - 0xd800u < 0x800)
			return NULL;
		else if (c < 0x10000) {
			*wcs++ = 224 + c / (64 * 64);
			*wcs++ = 128 + c / (64) % 64;
			*wcs++ = 128 + c % 64;
		}
		else if (c < 0x110000) {
			*wcs++ = 240 + c / (64 * 64 * 64);
			*wcs++ = 128 + c / (64 * 64) % 64;
			*wcs++ = 128 + c / (64) % 64;
			*wcs++ = 128 + c % 64;
		}
		else
			return NULL;
	}
	while (*src++ != '\0');

	return dest;
}

wchar_t *u_mbstowcs(const char *src) {
#define IS_IN_RANGE(c, f, l) (((c) >= (f)) && ((c) <= (l)))
#define RETURN_IF(b) if (b) { free(wcs); return NULL; }

	struct string;
	struct string {
		wchar_t c;
		struct string *next;
	};
	typedef unsigned long ulong;
	typedef unsigned char uchar;
	typedef struct string string;

	if (!src)
		return NULL;

	uchar c1, c2, *ptr = (uchar *)src;
	ulong uc = 0ul;
	int seqlen = 0;

	size_t size = 0llu, i;
	string *wcs = calloc(1, sizeof(string));
	string *current = wcs;

	while (*ptr != '\0') {
		ptr += seqlen;

		c1 = ptr[0];

		if ((c1 & 0x80) == 0) {
			uc = (ulong)(c1 & 0x7F);
			seqlen = 1;
		}
		else if ((c1 & 0xE0) == 0xC0) {
			uc = (ulong)(c1 & 0x1F);
			seqlen = 2;
		}
		else if ((c1 & 0xF0) == 0xE0) {
			uc = (ulong)(c1 & 0x0F);
			seqlen = 3;
		}
		else if ((c1 & 0xF8) == 0xF0) {
			uc = (ulong)(c1 & 0x07);
			seqlen = 4;
		}
		else {
			RETURN_IF (1)
		}

		for (i = 1llu; i < seqlen; ++i) {
			c1 = ptr[i];
			RETURN_IF ((c1 & 0xC0) != 0x80)
		}

		switch (seqlen) {
			case 2: {
				c1 = ptr[0];
				RETURN_IF (!IS_IN_RANGE(c1, 0xC2, 0xDF))
				break;
			}
			case 3: {
				c1 = ptr[0];
				c2 = ptr[1];

				switch (c1) {
					case 0xE0: RETURN_IF (!IS_IN_RANGE(c2, 0xA0, 0xBF)) break;
					case 0xED: RETURN_IF (!IS_IN_RANGE(c2, 0x80, 0x9F)) break;
					default: RETURN_IF (!IS_IN_RANGE(c1, 0xE1, 0xEC) && !IS_IN_RANGE(c1, 0xEE, 0xEF)) break;
				}
				break;
			}

			case 4: {
				c1 = ptr[0];
				c2 = ptr[1];

				switch (c1) {
					case 0xF0: RETURN_IF (!IS_IN_RANGE(c2, 0x90, 0xBF)) break;
					case 0xF4: RETURN_IF (!IS_IN_RANGE(c2, 0x80, 0x8F)) break;
					default: RETURN_IF (!IS_IN_RANGE(c1, 0xF1, 0xF3)) break;
				}
				break;
			}
		}

		for (i = 1llu; i < seqlen; ++i)
			uc = ((uc << 6) | (ulong)(ptr[i] & 0x3F));

		current->c = uc;
		current->next = calloc(1, sizeof(string));
		if (!current->next)
			exit(EXIT_FAILURE);
		current = current->next;

		size += 1;
	}

	wchar_t *dest = calloc(size, sizeof(wchar_t));
	if (!dest)
		exit(EXIT_FAILURE);

	current = wcs;
	for (i = 0llu; i < size; ++i) {
		dest[i] = current->c;
		string *prev = current;
		current = current->next;
		free(prev);
	}

	return dest;
}

wchar_t *u_wcstolower(wchar_t *str) {
	wchar_t *current = str;
	while (*current++ != '\0')
		*current = towlower(*current);
	return str;
}

wchar_t *u_wcstolower_s(const wchar_t *str, size_t size) {
	wchar_t *str_lower = malloc(size * sizeof(wchar_t));
	size_t i;
	for (i = 0llu; i < size; ++i)
		str_lower[i] = towlower(str[i]);
	return str_lower;
}

int u_is_number(const char *str, int *out)
{
	int number = 0;
	for (; *str != '\0'; ++str) {
		int code = (int)*str;
		if (code < 48 || code > 57)
			return 0;
		number *= 10;
		number += code - 48;
	}
	*out = number;
	return 1;
}

zval u_call_function(zval *object, const char *name, zval *params, uint32_t param_count)
{
	zval function_name, return_value;
	ZVAL_STRING(&function_name, name);
	call_user_function(&Z_CE_P(object)->function_table, object, &function_name, &return_value, param_count, params);
	return return_value;
}

HashTable *u_create_table(uint32_t size)
{
	HashTable *ht;
	ALLOC_HASHTABLE(ht);
	zend_hash_init(ht, size, NULL, ZVAL_PTR_DTOR, 0);
	return ht;
}
