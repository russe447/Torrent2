/*
 * C implementation of a bencode decoder.
 * This is the format defined by BitTorrent:
 *  http://wiki.theory.org/BitTorrentSpecification#bencoding
 *
 * The only external requirements are a few [standard] function calls and
 * the long long type.  Any sane system should provide all of these things.
 *
 * See the bencode.h header file for usage information.
 *
 * This is released into the public domain:
 *  http://en.wikipedia.org/wiki/Public_Domain
 *
 * Written by:
 *   Mike Frysinger <vapier@gmail.com>
 * And improvements from:
 *   Gilles Chanteperdrix <gilles.chanteperdrix@xenomai.org>
 */

/*
 * This implementation isn't optimized at all as I wrote it to support a bogus
 * system.  I have no real interest in this format.  Feel free to send me
 * patches (so long as you don't copyright them and you release your changes
 * into the public domain as well).
 */

#include <stdio.h> /* printf() */
#include <stdlib.h> /* malloc() realloc() free() strtoll() */
#include <string.h> /* memset() */

#include "bencode.h"

#ifndef BE_DEBUG
#define BE_DEBUG 0
#endif

#define EAT(BUF,LEN) (BUF)++,(LEN)--
#define EAT_N(BUF,LEN,N) (BUF)+=(N),(LEN)-=(N)
#define TMPBUFLEN 32

#define CHECK_AND_UPDATE { if (r < 0) return -1; if (outBuf) EAT_N(outBuf, outBufLen, r); sz += r;} 


#define DBG(fmt, args...) do { /*printf(fmt, ## args);*/} while (0)
static inline void dump_string(const char *str, long long len)
{
	long long i;
	const unsigned char *s = (const unsigned char *)str;

	/* Assume non-ASCII data is binary. */
	for (i = 0; i < len; ++i)
		if (s[i] >= 0x20 && s[i] <= 0x7e)
			printf("%c", s[i]);
		else
			printf("\\x%02x", s[i]);
}
#define DUMP_STRING(str, len) do {  /*dump_string(str, len); */} while (0)

static be_node *be_alloc(be_type type)
{
	be_node *ret = malloc(sizeof(*ret));
	if (ret) {
		memset(ret, 0x00, sizeof(*ret));
		ret->type = type;
	}
	return ret;
}

static long long _be_decode_int(const char **data, long long *data_len)
{
	char *endp;
	long long ret = strtoll(*data, &endp, 10);
	*data_len -= (endp - *data);
	*data = endp;
	return ret;
}

long long be_str_len(be_node *node)
{
	long long ret = 0;
	if (node->val.s)
		memcpy(&ret, node->val.s - sizeof(ret), sizeof(ret));
	return ret;
}

static char *_be_decode_str(const char **data, long long *data_len)
{
	long long sllen = _be_decode_int(data, data_len);
	long slen = sllen;
	unsigned long len;
	char *ret = NULL;

	/* slen is signed, so negative values get rejected */
	if (sllen < 0)
		return ret;

	/* reject attempts to allocate large values that overflow the
	 * size_t type which is used with malloc()
	 */
	if (sizeof(long long) != sizeof(long))
		if (sllen != slen)
			return ret;

	/* make sure we have enough data left */
	if (sllen > *data_len - 1)
		return ret;

	/* switch from signed to unsigned so we don't overflow below */
	len = slen;

	if (**data == ':') {
		char *_ret = malloc(sizeof(sllen) + len + 1);
		memcpy(_ret, &sllen, sizeof(sllen));
		ret = _ret + sizeof(sllen);
		memcpy(ret, *data + 1, len);
		ret[len] = '\0';
		*data += len + 1;
		*data_len -= len + 1;

		DUMP_STRING(ret, sllen);
	}
	return ret;
}

static be_node *_be_decode(const char **data, long long *data_len)
{
	be_node *ret = NULL;

	if (!*data_len)
		return ret;

	switch (**data) {
		/* lists */
		case 'l': {
			unsigned int i = 0;

			DBG("%p: decoding list ...\n", *data);

			ret = be_alloc(BE_LIST);

			--(*data_len);
			++(*data);
			while (**data != 'e') {
				ret->val.l = realloc(ret->val.l, (i + 2) * sizeof(*ret->val.l));
				ret->val.l[i] = _be_decode(data, data_len);
				if (!ret->val.l[i])
					break;
				++i;
			}
			--(*data_len);
			++(*data);

			/* In case of an empty list. Uncommon, but valid. */
			if (!i)
				ret->val.l = realloc(ret->val.l, sizeof(*ret->val.l));

			ret->val.l[i] = NULL;

			return ret;
		}

		/* dictionaries */
		case 'd': {
			unsigned int i = 0;

			DBG("%p: decoding dict ...\n", *data);

			ret = be_alloc(BE_DICT);

			--(*data_len);
			++(*data);
			while (**data != 'e') {
				ret->val.d = realloc(ret->val.d, (i + 2) * sizeof(*ret->val.d));
				DBG("  [%i] key: ", i);
				ret->val.d[i].key = _be_decode_str(data, data_len);
				DBG("\n");
				ret->val.d[i].val = _be_decode(data, data_len);
				if (!ret->val.l[i])
					break;
				++i;
			}
			--(*data_len);
			++(*data);

			ret->val.d[i].val = NULL;

			return ret;
		}

		/* integers */
		case 'i': {
			DBG("%p: decoding int: ", *data);

			ret = be_alloc(BE_INT);

			--(*data_len);
			++(*data);
			ret->val.i = _be_decode_int(data, data_len);
			if (**data != 'e') {
				DBG("invalid value; rejecting it\n");
				be_free(ret);
				return NULL;
			}
			--(*data_len);
			++(*data);

			DBG("%lli\n", ret->val.i);

			return ret;
		}

		/* byte strings */
		case '0'...'9': {
			DBG("%p: decoding byte string (%c): ", *data, **data);

			ret = be_alloc(BE_STR);
			ret->val.s = _be_decode_str(data, data_len);

			DBG("\n");

			return ret;
		}

		/* invalid */
		default:
			break;
	}

	return ret;
}

be_node *be_decoden(const char *data, long long len)
{
	return _be_decode(&data, &len);
}

be_node *be_decode(const char *data)
{
	return be_decoden(data, strlen(data));
}

static inline void _be_free_str(char *str)
{
	if (str)
		free(str - sizeof(long long));
}
void be_free(be_node *node)
{
	switch (node->type) {
		case BE_STR:
			_be_free_str(node->val.s);
			break;

		case BE_INT:
			break;

		case BE_LIST: {
			unsigned int i;
			for (i = 0; node->val.l[i]; ++i)
				be_free(node->val.l[i]);
			free(node->val.l);
			break;
		}

		case BE_DICT: {
			unsigned int i;
			for (i = 0; node->val.d[i].val; ++i) {
				_be_free_str(node->val.d[i].key);
				be_free(node->val.d[i].val);
			}
			free(node->val.d);
			break;
		}
	}
	free(node);
}


static void _be_dump_indent(ssize_t indent)
{
	while (indent-- > 0)
		printf("    ");
}
static void _be_dump(be_node *node, ssize_t indent)
{
	size_t i;

	_be_dump_indent(indent);
	indent = abs(indent);

	switch (node->type) {
		case BE_STR: {
			long long len = be_str_len(node);
			printf("str = ");
			DUMP_STRING(node->val.s, len);
			printf(" (len = %lli)\n", len);
			break;
		}

		case BE_INT:
			printf("int = %lli\n", node->val.i);
			break;

		case BE_LIST:
			puts("list [");

			for (i = 0; node->val.l[i]; ++i)
				_be_dump(node->val.l[i], indent + 1);

			_be_dump_indent(indent);
			puts("]");
			break;

		case BE_DICT:
			puts("dict {");

			for (i = 0; node->val.d[i].val; ++i) {
				_be_dump_indent(indent + 1);
				printf("%s => ", node->val.d[i].key);
				_be_dump(node->val.d[i].val, -(indent + 1));
			}

			_be_dump_indent(indent);
			puts("}");
			break;
	}
}
void be_dump(be_node *node)
{
	_be_dump(node, 0);
}

static ssize_t be_encode_str(const char *str, char *outBuf, size_t outBufLen) {
	long long ret = 0;
	memcpy(&ret, str - sizeof(ret), sizeof(ret));
    char tmpBuf[TMPBUFLEN];
    int sz = snprintf(tmpBuf, TMPBUFLEN, "%lld:", ret);
    if (outBuf == NULL){
		return sz + ret;
	}
        

    if (sz + ret > outBufLen){
		return -1;
	}
    memcpy(outBuf, tmpBuf, sz);
    memcpy(outBuf+sz, str, ret);
    return sz + ret;
}

/*
  when outBuf == NULL, be_encode returns outBufLen needed 
*/

ssize_t be_encode(const be_node *node, char *outBuf, size_t outBufLen) {
    char tmpBuf[TMPBUFLEN];
    ssize_t r;
    int sz = 1;

    if (outBuf && outBufLen == 0)
        return -1;
    
    switch (node->type) {
    case BE_INT:
        sz = snprintf(tmpBuf, TMPBUFLEN, "i%llde", node->val.i);
        if (outBuf != NULL) {
            if (sz > outBufLen)
                return -1;
            strncpy(outBuf, tmpBuf, sz);
        }
        break;
    case BE_STR:
        sz = be_encode_str(node->val.s, outBuf, outBufLen); 
        if (sz < 0)
            return -1;
        break;
    case BE_LIST:
        if (outBuf) {
            *outBuf = 'l';
            EAT(outBuf, outBufLen);
        }
		for(int j = 0; node->val.l[j]; ++j){
			r = be_encode(node->val.l[j], outBuf, outBufLen);
			CHECK_AND_UPDATE;
		}
        if (outBuf) {
            if (outBufLen == 0)
                return -1;
            *outBuf = 'e';
        }
        sz++;
        break;
    case BE_DICT:
        if (outBuf) {
            *outBuf = 'd';
            EAT(outBuf, outBufLen);
        }
		for (int i = 0; node->val.d[i].val; ++i) {
			r = be_encode_str(node->val.d[i].key, outBuf, outBufLen);
			CHECK_AND_UPDATE;
		
			r = be_encode(node->val.d[i].val, outBuf, outBufLen);
			CHECK_AND_UPDATE;
		}
			
        if (outBuf) {
            if (outBufLen == 0)
                return -1;
            *outBuf = 'e';
        }
        sz++;
        break;
    }
    return sz;
}
