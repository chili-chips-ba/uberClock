// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>

extern "C" void hellocpp(void);
void hellocpp(void)
{
    printf("C++: Hello, world!\n");
}