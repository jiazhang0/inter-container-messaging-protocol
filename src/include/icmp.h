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

#ifndef ICMP_H
#define ICMP_H

#include <eee.h>
#include <ic_error.h>

#ifndef ICMP_ERRNO_BASE
  #error "ICMP_ERRNO_BASE isn't defined"
#endif

#define ICMP_VERSION		1

#ifndef ICMP_CHANNEL_PREFIX
  #define ICMP_CHANNEL_PREFIX		"/opt/container/"
#endif

#pragma pack (1)

/* V0 header is only used for parsing message header internally.
 * Don't use it. Really!
 */
typedef struct {
	uint8_t version;
	uint16_t header_length;
	uint16_t command_code;
} icmp_message_v0_header_t;

typedef struct {
	uint8_t version;
	uint16_t header_length;
	uint16_t command_code;
	uint32_t payload_length;
	uint32_t authorization_length;
	uint8_t reserved[3];		/* must be zeroed */
} icmp_message_v1_header_t;

typedef struct {
	icmp_message_v1_header_t header;
	uint8_t parameters[0];
} icmp_message_v1_t;

typedef union {
	icmp_message_v0_header_t v0;
	icmp_message_v1_t v1;
} icmp_message_t;

#pragma pack (0)

#define ICMP_CC_ECHO			0
#define ICMP_CC_COMMMANDLINE		1
#define ICMP_MAX_CC			(ICMP_CC_COMMMANDLINE + 1)
#define ICMP_CC_NOT_SPECIFIED		0xffffU

static inline uint8_t
icmp_message_version(void)
{
	return ICMP_VERSION;
}

static inline uint8_t
icmp_message_header_length(uint8_t ver)
{
	switch (ver) {
	case 1:
		return sizeof(icmp_message_v1_header_t);
	case 0:
		return sizeof(icmp_message_v0_header_t);
	}

	return 0;
}

extern int
icmp_marshal(void *data, unsigned long data_len, uint32_t cc,
	     void **ret_msg, unsigned long *ret_msg_len);

extern int
icmp_unmarshal(void *msg, unsigned long msg_len, uint16_t cc,
	       int (*handler)(void *ctx, uint16_t cc, const void *payload,
			      unsigned long payload_len),
	       void *handler_ctx);

#endif	/* ICMP_H */
