#ifndef CONV_H
#define CONV_H

#include <stddef.h>
#include <wchar.h>

char* wcs_to_mbs(const wchar_t *src);
wchar_t* mbs_to_wcs(const char *src);
wchar_t* wcs_to_lower(wchar_t *str);
wchar_t* wcs_to_lower_s(const wchar_t *str, size_t size);

#endif // !CONV_H
