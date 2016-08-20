/*
 * Inter-Container specific error code
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#ifndef IC_ERROR_H
#define IC_ERROR_H

typedef unsigned long		ic_error_t;

/* The Inter-Container specific error code must be ranged in a nonoverlapping
 * area against the error codes defined by other components, e.g, nanomsg
 * and glibc.
 */
#ifndef IC_ERRNO_BASE
  #define IC_ERRNO_BASE				0xdeadfaceUL
#else
  #error "IC_ERRNO_BASE must not be defined as 0"
#endif

/* The errno interval between any two subsystems */
#define IC_ERRNO_OFFSET				0x1000

/* The common errno used by any subsystem */
#define IC_ERRNO(err)				(IC_ERRNO_BASE + (err))
#define IC_ERRNO_NONE				IC_ERRNO(0)
#define IC_ERRNO_INVALID_PARAMETER		IC_ERRNO(1)
#define IC_ERRNO_OUT_OF_MEM			IC_ERRNO(2)
#define IC_ERRNO_COMMAND_DENIED			IC_ERRNO(3)

#define BYTE_STREAM_ERRNO_BASE			(IC_ERRNO_BASE + IC_ERRNO_OFFSET)
#define VECTOR_ERRNO_BASE			(BYTE_STREAM_ERRNO_BASE + IC_ERRNO_OFFSET)
#define NANOMSG_ERRNO_BASE			(VECTOR_ERRNO_BASE + IC_ERRNO_OFFSET)
#define STRING_TREE_ERRNO_BASE			(NANOMSG_ERRNO_BASE + IC_ERRNO_OFFSET)
#define YAML_ERRNO_BASE				(STRING_TREE_ERRNO_BASE + IC_ERRNO_OFFSET)
#define ICMP_ERRNO_BASE				(YAML_ERRNO_BASE + IC_ERRNO_OFFSET)
#define IC_CONF_ERRNO_BASE			(ICMP_ERRNO_BASE + IC_ERRNO_OFFSET)

extern void
ic_set_errno(ic_error_t e);

extern ic_error_t
ic_get_errno(void);

static int inline
ic_error(void)
{
	ic_error_t e = ic_get_errno();
	return e >= IC_ERRNO_BASE && e != IC_ERRNO_NONE;
}

#endif	/* IC_ERROR_H */
