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

#include <buffer_stream.h>

static long
bs_abs_offset(buffer_stream_t *bs, long offset)
{
	if (offset < 0)
		return bs->buf_len + offset;

	return offset;
}

void
bs_init(buffer_stream_t *bs, void *buf, unsigned long buf_len)
{
	bs->buf = buf;
	bs->buf_len = buf_len;
	bs->current = 0;
}

unsigned long
bs_tell(buffer_stream_t *bs)
{
	return bs->current;
}

unsigned long
bs_size(buffer_stream_t *bs)
{
	return bs->buf_len;
}

unsigned long
bs_empty(buffer_stream_t *bs)
{
	return !bs_size(bs);
}

unsigned long
bs_remain(buffer_stream_t *bs)
{
	return bs->buf_len - bs->current;
}

void *
bs_head(buffer_stream_t *bs)
{
	return bs->buf;
}

int
bs_seek(buffer_stream_t *bs, long offset)
{
	long new_offset = (long)bs->current + offset;

	if (new_offset < 0 || new_offset >= (long)bs->buf_len) {
		bs_set_errno(BYTE_STREAM_ERRNO_INVALID_PARAMETER);
		return -1;
	}

	bs->current += offset;

	return 0;
}

int
bs_seek_at(buffer_stream_t *bs, long offset)
{
	offset = bs_abs_offset(bs, offset);
	if (offset < 0 || offset >= bs->buf_len) {
		bs_set_errno(BYTE_STREAM_ERRNO_OUT_OF_RANGE);
		return -1;
	}

	bs->current = offset;

	return 0;
}

int
bs_reserve_at(buffer_stream_t *bs, unsigned long ext_len, long offset)
{
	unsigned long out_buflen = offset + ext_len;

	if (out_buflen > bs->buf_len) {
		void *out_buf;

		out_buf = eee_malloc(out_buflen);
		// out_buf = eee_mrealloc_aligned(bs->buf, bs->buf_len, &out_buflen);
		if (!out_buf) {
			err("Failed to extend out buffer\n");
			bs_set_errno(BYTE_STREAM_ERRNO_OUT_OF_MEM);
			return -1;
		}

		eee_memcpy(out_buf, bs->buf, bs->buf_len);

		bs->buf = out_buf;
		bs->buf_len = out_buflen;
	}

	return bs_seek_at(bs, offset);
}

int
bs_reserve(buffer_stream_t *bs, unsigned long ext_len)
{
	return bs_reserve_at(bs, ext_len, bs->current);
}

static int
__bs_get_at(buffer_stream_t *bs, void **in, unsigned long in_len,
	    int post, long offset)
{
	offset = bs_abs_offset(bs, offset);
	if (offset < 0 || offset + in_len > bs->buf_len) {
		bs_set_errno(BYTE_STREAM_ERRNO_OUT_OF_RANGE);
		return -1;
	}

	if (in)
		*in = bs->buf + offset;

	bs->current = offset;

	if (post)
		bs->current += in_len;

	return 0;
}

int
bs_post_get_at(buffer_stream_t *bs, void **in, unsigned long in_len,
	       long offset)
{
	return __bs_get_at(bs, in, in_len, 1, offset);
}

int
bs_get_at(buffer_stream_t *bs, void **in, unsigned long in_len,
	  long offset)
{
	return __bs_get_at(bs, in, in_len, 0, offset);
}

int
bs_post_get(buffer_stream_t *bs, void **in, unsigned long in_len)
{
	return __bs_get_at(bs, in, in_len, 1, bs->current);
}

int
bs_get(buffer_stream_t *bs, void **in, unsigned long in_len)
{
	return __bs_get_at(bs, in, in_len, 0, bs->current);
}

static int
__bs_put_at(buffer_stream_t *bs, void *out, unsigned long out_len,
	    int post, long offset)
{
	offset = bs_abs_offset(bs, offset);
	if (offset < 0 || offset + out_len > bs->buf_len) {
		bs_set_errno(BYTE_STREAM_ERRNO_OUT_OF_RANGE);
		return -1;
	}

	if (out)
		eee_memcpy(bs->buf + offset, out, out_len);

	bs->current = offset;

	if (post)
		bs->current += out_len;

	return 0;
}

int
bs_post_put_at(buffer_stream_t *bs, void *out, unsigned long out_len,
	       long offset)
{
	return __bs_put_at(bs, out, out_len, 1, offset);
}

int
bs_put_at(buffer_stream_t *bs, void *out, unsigned long out_len,
	  long offset)
{
	return __bs_put_at(bs, out, out_len, 0, offset);
}

int
bs_post_put(buffer_stream_t *bs, void *out, unsigned long out_len)
{
	return __bs_put_at(bs, out, out_len, 1, bs->current);
}

int
bs_put(buffer_stream_t *bs, void *out, unsigned long out_len)
{
	return __bs_put_at(bs, out, out_len, 0, bs->current);
}

void
bs_destroy(buffer_stream_t *bs)
{
	eee_mfree(bs->buf);
}