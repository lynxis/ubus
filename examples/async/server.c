/*
 * Copyright (C) 2023 Alexander Couzens <lynxis@fe80.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <unistd.h>
#include <signal.h>

#include <libubox/blobmsg_json.h>
#include "libubus.h"

static struct ubus_context *ctx;
static struct ubus_subscriber test_event;
static struct blob_buf b;

enum {
	ECHO_MSG,
	__ECHO_MAX
};

static const struct blobmsg_policy echo_policy[] = {
	[ECHO_MSG] = { .name = "message", .type = BLOBMSG_TYPE_STRING },
};

struct echo_request {
	struct ubus_request_data req;
	struct uloop_timeout timeout;
	int fd;
	int idx;
	char data[];
};

static void echo_method_async(struct uloop_timeout *t);

static int echo_method(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__ECHO_MAX];
	const char format[] = "%s received a message: %s";
	const char *msgstr = "(unknown)";
	char *reply_msg;

	blobmsg_parse(echo_policy, ARRAY_SIZE(echo_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[ECHO_MSG])
		msgstr = blobmsg_data(tb[ECHO_MSG]);

	size_t len = sizeof(format) + strlen(obj->name) + strlen(msgstr) + 1;
	reply_msg = calloc(1, len);
	if (!reply_msg)
		return UBUS_STATUS_UNKNOWN_ERROR;

	snprintf(reply_msg, len, format, obj->name, msgstr);

	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "message", reply_msg);
	ubus_send_reply(ctx, req, b.head);
	free(reply_msg);

	return 0;
}

static int long_echo_method(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct echo_request *hreq;
	struct blob_attr *tb[__ECHO_MAX];
	const char format[] = "%s received a message: %s";
	const char *msgstr = "(unknown)";

	blobmsg_parse(echo_policy, ARRAY_SIZE(echo_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[ECHO_MSG])
		msgstr = blobmsg_data(tb[ECHO_MSG]);

	size_t len = sizeof(*hreq) + sizeof(format) + strlen(obj->name) + strlen(msgstr) + 1;
	hreq = calloc(1, len);
	if (!hreq)
		return UBUS_STATUS_UNKNOWN_ERROR;

	snprintf(hreq->data, len, format, obj->name, msgstr);
	ubus_defer_request(ctx, req, &hreq->req);
	hreq->timeout.cb = echo_method_async;
	uloop_timeout_set(&hreq->timeout, 5000);
	fprintf(stderr, "Done with callback. Msg: '%s'. The callback will comeback in 5 seconds.\n", hreq->data);

	return 0;
}

static void echo_method_async(struct uloop_timeout *t)
{
	struct echo_request *req = container_of(t, struct echo_request, timeout);

	fprintf(stderr, "In async: '%s'\n", req->data);
	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "message", req->data);
	ubus_send_reply(ctx, &req->req, b.head);
	ubus_complete_deferred_request(ctx, &req->req, 0);
	free(req);
}

static const struct ubus_method async_methods[] = {
	UBUS_METHOD("echo", echo_method, echo_policy),
	UBUS_METHOD("longecho", long_echo_method, echo_policy),
};

static struct ubus_object_type async_object_type =
	UBUS_OBJECT_TYPE("async", async_methods);

static struct ubus_object async_object = {
	.name = "async",
	.type = &async_object_type,
	.methods = async_methods,
	.n_methods = ARRAY_SIZE(async_methods),
};

static void server_main(void)
{
	int ret;

	ret = ubus_add_object(ctx, &async_object);
	if (ret)
		fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));

	ret = ubus_register_subscriber(ctx, &test_event);
	if (ret)
		fprintf(stderr, "Failed to add watch handler: %s\n", ubus_strerror(ret));

	uloop_run();
}

int main(int argc, char **argv)
{
	const char *ubus_socket = NULL;
	int ch;

	while ((ch = getopt(argc, argv, "s:")) != -1) {
		switch (ch) {
		case 's':
			ubus_socket = optarg;
			break;
		default:
			break;
		}
	}

	uloop_init();
	signal(SIGPIPE, SIG_IGN);

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);
	server_main();
	ubus_free(ctx);
	uloop_done();

	return 0;
}
