/* json-xml.c
   Copyright (c) 2017, Sijmen J. Mulder <ik@sjmulder.nl>
   See LICENSE.txt */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <yajl/yajl_parse.h>

#define NAME "json-xml"

#define LEN(arr)    (sizeof(arr)/sizeof(arr[0]))
#define CURKEY(ctx) (ctx->keys[ctx->icurkey])

struct ctx {
	char keystack[4096]; /* key0[] key1[] key2[] ... */
	char *keys[256];     /* &key0 &key1 &key2 &key3 (!) ... */
	int icurkey;         /* 2 */
	int indent;
};

static void pushkey(struct ctx *ctx, const char *key)
{
	int len;

	len = strlen(key);

	if (ctx->icurkey+2 >= LEN(ctx->keys)) {
		fprintf(stderr, NAME ": maximum key depth exceeded\n");
		exit(EXIT_FAILURE);
	}

	if (ctx->keys[ctx->icurkey+1]+len+1 >=
			ctx->keystack + sizeof(ctx->keystack)) {
		fprintf(stderr, NAME ": key storage space exceeded\n");
		exit(EXIT_FAILURE);
	}

	ctx->icurkey++;
	ctx->keys[ctx->icurkey+1] = CURKEY(ctx) + len+1;
	strncpy(CURKEY(ctx), key, len);
	CURKEY(ctx)[len] = '\0';
}

static void popkey(struct ctx *ctx)
{
	assert(ctx->icurkey > 0);
	ctx->icurkey--;
}

static const char *sanitize(const char *s, int n)
{
	static char buf[1024];
	int i, len;

	if (!s[0])
		return "_";

	i = 0;
	len = 1;
	buf[0] = isalpha(s[0]) ? s[i++] : '_';

	for (; i<n && s[i] && len < sizeof(buf)-1; i++) {
		if (isalnum(s[i]))
			buf[len++] = s[i];
		else if (buf[len-1] != '_')
			buf[len++] = '_';
	}

	if (len>1 && buf[len-1] == '_')
		len--;

	buf[len] = '\0';
	return buf;
}

static void pindent(int n)
{
	while (n--)
		printf("  ");
}

static int handle_null(void *vp)
{
	struct ctx *ctx;

	ctx = vp;
	pindent(ctx->indent);
	printf("<%s type=\"null\" />\n", CURKEY(ctx));
	return 1;
}

static int handle_boolean(void *vp, int val)
{
	struct ctx *ctx;

	ctx = vp;
	pindent(ctx->indent);
	printf("<%s type=\"boolean\">%s</%s>\n", CURKEY(ctx),
		val ? "true" : "false", CURKEY(ctx));
	return 1;
}

static int handle_integer(void *vp, long long val)
{
	struct ctx *ctx;

	ctx = vp;
	pindent(ctx->indent);
	printf("<%s type=\"number\">%lld</%s>\n", CURKEY(ctx), val,
		CURKEY(ctx));
	return 1;
}

static int handle_double(void *vp, double val)
{
	struct ctx *ctx;

	ctx = vp;
	pindent(ctx->indent);
	printf("<%s type=\"number\">%f</%s>\n", CURKEY(ctx), val,
		CURKEY(ctx));
	return 1;
}

static int handle_number(void *vp, const char *str, size_t len)
{
	struct ctx *ctx;

	ctx = vp;
	pindent(ctx->indent);
	printf("<%s type=\"number\">", CURKEY(ctx));
	fwrite(str, 1, len, stdout);
	printf("</%s>\n", CURKEY(ctx));
	return 1;
}

static int handle_string(void *vp, const unsigned char *str, size_t len)
{
	struct ctx *ctx;
	int i;

	ctx = vp;
	pindent(ctx->indent);
	printf("<%s type=\"string\">", CURKEY(ctx));

	for (i = 0; i<len && str[i]; i++) {
		if (str[i] == '<')
			fputs("&lt;", stdout);
		else if (str[i] == '>')
			fputs("&gt;", stdout);
		else if (str[i] == '&')
			fputs("&amp;", stdout);
		else
			putchar(str[i]);
	}

	printf("</%s>\n", CURKEY(ctx));
	return 1;
}

static int handle_map_start(void *vp)
{
	struct ctx *ctx;

	ctx = vp;
	pindent(ctx->indent++);
	printf("<%s type=\"object\">\n", CURKEY(ctx));
	pushkey(ctx, ""); /* dummy for first handle_map_key to pop */
	return 1;
}

static int handle_map_key(void *vp, const unsigned char *str, size_t len)
{
	popkey(vp); /* previous key (or dummy) */
	pushkey(vp, sanitize(str, len));
	return 1;
}

static int handle_map_end(void *vp)
{
	struct ctx *ctx;

	ctx = vp;
	popkey(ctx); /* previous key */
	pindent(--ctx->indent);
	printf("</%s>\n", CURKEY(ctx));
	return 1;
}

static int handle_array_start(void *vp)
{
	struct ctx *ctx;

	ctx = vp;
	pindent(ctx->indent++);
	printf("<%s type=\"array\">\n", CURKEY(ctx));
	pushkey(ctx, "item");
	return 1;
}

static int handle_array_end(void *vp)
{
	struct ctx *ctx;

	ctx = vp;
	popkey(ctx); /* "item" */
	pindent(--ctx->indent);
	printf("</%s>\n", CURKEY(ctx));
	return 1;
}

static const yajl_callbacks callbacks = {
	handle_null,
	handle_boolean,
	handle_integer,
	handle_double,
	handle_number,
	handle_string,
	handle_map_start,
	handle_map_key,
	handle_map_end,
	handle_array_start,
	handle_array_end
};

int main(int argc, char **argv)
{
	struct ctx ctx;
	yajl_handle yajl;
	yajl_status ys;
	unsigned char buf[4096];
	size_t num;

	if (isatty(fileno(stdin))) {
		printf("usage: " NAME " <file.json\n");
		return 0;
	}

	memset(&ctx, 0, sizeof(ctx));
	ctx.keys[0] = ctx.keystack;
	ctx.icurkey = -1;
	pushkey(&ctx, "root");

	if (!(yajl = yajl_alloc(&callbacks, NULL, &ctx))) {
		fprintf(stderr, NAME ": failed to initialize libyajl\n");
		exit(EXIT_FAILURE);
	}

	printf("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");

	while ((num = fread(buf, 1, sizeof(buf), stdin)) > 0) {
		if ((ys = yajl_parse(yajl, buf, num))) {
			fprintf(stderr, NAME ": %s",
				yajl_status_to_string(ys));
			exit(EXIT_FAILURE);
		}
	}

	if (ferror(stdin)) {
		perror(NAME);
		exit(EXIT_FAILURE);
	}

	if ((ys = yajl_complete_parse(yajl))) {
		fprintf(stderr, NAME ": %s", yajl_status_to_string(ys));
		exit(EXIT_FAILURE);
	}

	return 0;
}
