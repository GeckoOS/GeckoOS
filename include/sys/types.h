/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/*
 *	POSIX Standard: 2.6 Primitive System Data Types	<sys/types.h>
 */


#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H 1

#include <stddef.h>   /* size_t, ptrdiff_t */
#include <stdint.h>   /* uint*_t, int*_t   */

/* ssize_t: signed counterpart of size_t */
#ifndef __ssize_t_defined
typedef long ssize_t;
# define __ssize_t_defined
#endif

/* off_t: file offset */
#ifndef __off_t_defined
typedef long long off_t;
# define __off_t_defined
#endif

/* pid_t, uid_t, gid_t — placeholder types */
typedef int  pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;

#endif /* _SYS_TYPES_H */
