/*
 * Buffer stream infrastructure
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#ifndef __BUFFER_STREAM_H__
#define __BUFFER_STREAM_H__

#ifdef EEE_IC
  #include <ic.h>
  #define bs_set_errno		ic_set_errno
#endif

#ifndef BYTE_STREAM_ERRNO_BASE
  #error "BYTE_STREAM_ERRNO_BASE isn't defined"
#endif

struct buffer_stream;

struct buffer_stream {
	void *buf;
	unsigned long buf_len;
	unsigned long current;
	void *in;
	unsigned long in_len;
	void *out;
	unsigned long out_len;
	struct buffer_stream *parent;
};

typedef struct buffer_stream		buffer_stream_t;

#ifndef BYTE_STREAM_ERRNO_BASE
  #define BYTE_STREAM_ERRNO_BASE		0xf000000UL
#endif

/* The errno used by byte stream */

#define BYTE_STREAM_ERRNO(err)			(BYTE_STREAM_ERRNO_BASE + (err))

#define BYTE_STREAM_ERRNO_INVALID_PARAMETER	BYTE_STREAM_ERRNO(0)
#define BYTE_STREAM_ERRNO_OUT_OF_RANGE		BYTE_STREAM_ERRNO(1)
#define BYTE_STREAM_ERRNO_OUT_OF_MEM		BYTE_STREAM_ERRNO(2)

extern int
bs_alloc(buffer_stream_t **bs);

extern void
bs_destroy(buffer_stream_t *bs);

extern void
bs_init(buffer_stream_t *bs, void *buf, unsigned long buf_len);

extern unsigned long
bs_tell(buffer_stream_t *bs);

extern unsigned long
bs_remain(buffer_stream_t *bs);

extern unsigned long
bs_size(buffer_stream_t *bs);

extern unsigned long
bs_empty(buffer_stream_t *bs);

extern int
bs_seek(buffer_stream_t *bs, long offset);

extern int
bs_seek_at(buffer_stream_t *bs, long offset);

extern void *
bs_head(buffer_stream_t *bs);

extern int
bs_reserve_at(buffer_stream_t *bs, unsigned long ext_len, long offset);

extern int
bs_reserve(buffer_stream_t *bs, unsigned long ext_len);

extern int
bs_shrink_at(buffer_stream_t *bs, unsigned long len, long offset);

extern int
bs_post_get_at(buffer_stream_t *bs, void **in, unsigned long in_len,
	       long offset);

extern int
bs_get_at(buffer_stream_t *bs, void **in, unsigned long in_len,
	  long offset);

extern int
bs_post_get(buffer_stream_t *bs, void **in, unsigned long in_len);

extern int
bs_get(buffer_stream_t *bs, void **in, unsigned long in_len);

extern int
bs_post_put_at(buffer_stream_t *bs, void *out, unsigned long out_len,
	       long offset);

extern int
bs_put_at(buffer_stream_t *bs, void *out, unsigned long out_len,
	  long offset);

extern int
bs_post_put(buffer_stream_t *bs, void *out, unsigned long out_len);

extern int
bs_put(buffer_stream_t *bs, void *out, unsigned long out_len);

#endif	/* __BUFFER_STREAM_H__ */
