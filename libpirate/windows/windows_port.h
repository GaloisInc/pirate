/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#ifndef _PIRATE_WINDOWS_PORT_H
#define _PIRATE_WINDOWS_PORT_H

#ifdef _WIN32

#include <Windows.h>

#ifndef REG_DWORD
#error "REG_DWORD must be defined"
#endif

#if REG_DWORD == REG_DWORD_LITTLE_ENDIAN
#define htobe16(x) _byteswap_ushort(x)
#define be16toh(x) _byteswap_ushort(x)
#define htobe32(x) _byteswap_ulong(x)
#define be32toh(x) _byteswap_ulong(x)
#else
#define htobe16(x) (x)
#define be16toh(x) (x)
#define htobe32(x) (x)
#define be32toh(x) (x)
#endif // REG_DWORD == REG_DWORD_LITTLE_ENDIAN

#endif // _WIN32

#endif // _PIRATE_WINDOWS_PORT_H