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

#pragma once

#include <gtest/gtest.h>

#ifdef _WIN32

#define ASSERT_CROSS_PLATFORM_ERROR(unix, windows) ASSERT_EQ(0, errno); ASSERT_EQ(windows, GetLastError())
#define ASSERT_CROSS_PLATFORM_NO_ERROR() ASSERT_CROSS_PLATFORM_ERROR(0, 0)
#define CROSS_PLATFORM_RESET_ERROR() errno = 0; SetLastError(0)

#else

#define ASSERT_CROSS_PLATFORM_ERROR(unix, windows) ASSERT_EQ(unix, errno)
#define ASSERT_CROSS_PLATFORM_NO_ERROR() ASSERT_CROSS_PLATFORM_ERROR(0, 0)
#define CROSS_PLATFORM_RESET_ERROR() errno = 0

#define WSAEINVAL (10022)

#endif