/*
 * Inter-Container Messaging Protocol (ICMP)
 *
 * Copyright (c) 2016, Lans Zhang
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <lans.zhang2008@gmail.com>
 */

#include <ic.h>

#ifndef ICMP_CHANNEL_PREFIX
  #define ICMP_CHANNEL_PREFIX		"/opt/container/"
#endif

#ifdef DEBUG
static void
dump_header(icmp_message_t *msg)
{
	dbg("Dump ICMP message header:\n");

	switch (msg->v0.version) {
	case 1: {
		icmp_message_v1_header_t *v1 = &msg->v1.header;

		dbg("  Version: %d\n", v1->version);
		dbg("  Header Length: %d-byte\n", v1->header_length);
		dbg("  Command Code: 0x%x\n", v1->command_code);
		dbg("  Payload Length: %d-byte\n", v1->payload_length);
		dbg("  Authorization Length: %d-byte\n", v1->authorization_length);

		break;
	}
	default:
		err("Unknown ICMP header: ver %d\n", msg->v0.version);
		return;
	}
}
#else
static void
dump_header(icmp_message_t *msg)
{
}
#endif

static int
generate_authorization_area(buffer_stream_t *msg)
{
	return 0;
}

static int
verify_authorization_area(buffer_stream_t *msg)
{
	return 0;
}

int
icmp_marshal(void *payload, unsigned long payload_len, uint32_t cc,
	     void **ret_msg, unsigned long *ret_msg_len)
{
	if (!ret_msg && !ret_msg_len)
		return -1;

	if (cc >= ICMP_MAX_CC)
		return -1;

	buffer_stream_t msg;
	bs_init(&msg, NULL, 0);

	unsigned long header_len;
	uint8_t ver = icmp_message_version();
	header_len = icmp_message_header_length(ver);
	int rc = 0;

	switch (cc) {
	case ICMP_CC_ECHO:
	case ICMP_CC_COMMMANDLINE: {
		if (!payload || !payload_len)
			return -1;

		dbg("Prepare to marshal commandline %s\n", (char *)payload);

		bs_reserve(&msg, header_len + payload_len);
		bs_put_at(&msg, payload, payload_len, header_len);
		break;
	}
	default:
		err("Unknown commmand code: 0x%x\n", cc);
		return -1;
	}

	bs_seek_at(&msg, 0);
	icmp_message_t *header;
	bs_get(&msg, (void **)&header, header_len);

	switch (ver) {
	case 1: {
		icmp_message_v1_header_t *v1 = &header->v1.header;

		v1->version = 1;
		v1->header_length = sizeof(*v1);
		v1->command_code = cc;
		v1->payload_length = payload_len;
		/* Authorization area will be filled later */
		v1->authorization_length = 0;
		eee_memset(v1->reserved, 0, sizeof(v1->reserved));

		bs_seek_at(&msg, 0);
		rc = generate_authorization_area(&msg);

		break;
	}
	}

	if (!rc) {
		if (ret_msg)
			*ret_msg = bs_head(&msg);

		if (ret_msg_len)
			*ret_msg_len = bs_size(&msg);
	}

	if (ic_util_verbose())
		dump_header(header);

	return rc;
}

static int
sanity_check_header(buffer_stream_t *bs, uint16_t cc)
{
	icmp_message_v0_header_t *v0;
	int rc;

	rc = bs_post_get(bs, (void **)&v0, sizeof(*v0));
	if (rc) {
		err("Unable to retrieve ICMP message header\n");
		return rc;
	}

	uint8_t ver = v0->version;
	if (!ver) {
		dbg("Invalid ICMP message version\n");
		return -1;
	}

	if (v0->command_code >= ICMP_MAX_CC) {
		dbg("Invalid ICMP message command code (0x%x)\n",
		    v0->command_code);
		return -1;
	}

	if (cc != (uint16_t)ICMP_CC_NOT_SPECIFIED && v0->command_code != cc) {
		err("The command code (0x%x) is not expected (0x%x)\n",
		    v0->command_code, cc);
		return -1;
	}

	icmp_message_t *msg = (icmp_message_t *)v0;
	uint16_t header_len = 0;
	uint32_t payload_len = 0, auth_len = 0;

	if (ver == 1) {
		icmp_message_v1_header_t *v1 = &msg->v1.header;

		header_len = v1->header_length;
		payload_len = v1->payload_length;
		auth_len = v1->authorization_length;
	}

	if (header_len < icmp_message_header_length(ver)) {
		dbg("Invalid ICMP message header length\n");
		return -1;
	}

	if (header_len + payload_len + auth_len > bs_size(bs)) {
		dbg("Invalid ICMP message payload/authorization length\n");
		return -1;
	}

	return 0;
}

int
icmp_unmarshal(void *msg, unsigned long msg_len, uint16_t cc,
	       int (*handler)(void *ctx, uint16_t cc, const void *payload,
			      unsigned long payload_len),
	       void *handler_ctx)
{
	if (!msg || !msg_len || !handler)
		return -1;

	buffer_stream_t bs;
	bs_init(&bs, msg, msg_len);

	icmp_message_t *header = (icmp_message_t *)msg;
	if (ic_util_verbose())
		dump_header(header);

	int rc = sanity_check_header(&bs, cc);
	if (rc)
		return rc;

	uint8_t ver = icmp_message_version();
	icmp_message_v0_header_t *v0 = (icmp_message_v0_header_t *)msg;
	if (v0->version != ver) {
		if (v0->version < ver) {
			info("Running ICMP message version (%d) higher than "
			     "the received message version (%d)\n", ver,
			     v0->version);
			ver = v0->version;
		} else
			warn("Running ICMP message version (%d) lower than "
			     "the received message version (%d)\n", ver,
			     v0->version);
	}

	rc = verify_authorization_area(&bs);
	if (rc)
		return -1;

	unsigned long payload_len = 0;

	if (ver == 1) {
		icmp_message_v1_header_t *v1 = &header->v1.header;

		payload_len = v1->payload_length;
	}

	if (cc == ICMP_CC_NOT_SPECIFIED)
		cc = v0->command_code;

	void *payload;

	switch (cc) {
	case ICMP_CC_ECHO:
	case ICMP_CC_COMMMANDLINE:
		bs_get_at(&bs, (void **)&payload, payload_len,
			  v0->header_length);
		rc = handler(handler_ctx, cc, payload, payload_len);
		break;
	}

	return rc;
}
