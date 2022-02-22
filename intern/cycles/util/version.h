/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#ifndef __UTIL_VERSION_H__
#define __UTIL_VERSION_H__

/* Cycles version number */

CCL_NAMESPACE_BEGIN

#define CYCLES_VERSION_MAJOR 1
#define CYCLES_VERSION_MINOR 13
#define CYCLES_VERSION_PATCH 0

#define CYCLES_MAKE_VERSION_STRING2(a, b, c) #a "." #b "." #c
#define CYCLES_MAKE_VERSION_STRING(a, b, c) CYCLES_MAKE_VERSION_STRING2(a, b, c)
#define CYCLES_VERSION_STRING \
  CYCLES_MAKE_VERSION_STRING(CYCLES_VERSION_MAJOR, CYCLES_VERSION_MINOR, CYCLES_VERSION_PATCH)

CCL_NAMESPACE_END

#endif /* __UTIL_VERSION_H__ */