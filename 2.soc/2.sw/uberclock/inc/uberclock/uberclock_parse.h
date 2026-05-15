//SPDX-FilecopyrightText:2026
//Ahmed Imamović Tarik Hamedović
//SPDX-License-Identifier:
//APGL-3.0-or-later

#ifndef UBERCLOCK_PARSE_H
#define UBERCLOCK_PARSE_H

#include <stdint.h>

unsigned uberclock_parse_unsigned(const char *text, unsigned max_value_exclusive, const char *what);
int uberclock_parse_signed(const char *text, int min_value, int max_value, const char *what);
int uberclock_parse_ipv4(const char *text, uint32_t *out_ip);

#endif

