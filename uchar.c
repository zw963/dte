#include "uchar.h"
#include "unicode.h"
#include "common.h"

static inline int u_seq_len(unsigned int first_byte)
{
	if (first_byte < 0x80)
		return 1;
	if (first_byte < 0xc0)
		return 0;
	if (first_byte < 0xe0)
		return 2;
	if (first_byte < 0xf0)
		return 3;

	// could be 0xf8 but RFC 3629 doesn't allow codepoints above 0x10ffff
	if (first_byte < 0xf5)
		return 4;
	return -1;
}

static inline int u_is_continuation(unsigned int uch)
{
	return (uch & 0xc0) == 0x80;
}

static inline int u_seq_len_ok(unsigned int uch, int len)
{
	return u_char_size(uch) == len;
}

/*
 * Len  Mask         Note
 * -------------------------------------------------
 * 1    0111 1111    Not supported by this function!
 * 2    0001 1111
 * 3    0000 1111
 * 4    0000 0111
 * 5    0000 0011    Forbidded by RFC 3629
 * 6    0000 0001    Forbidded by RFC 3629
 */
static inline unsigned int u_get_first_byte_mask(unsigned int len)
{
	return (1U << 7U >> len) - 1U;
}

unsigned int u_str_width(const unsigned char *str, unsigned int size)
{
	unsigned int idx = 0, w = 0;

	while (idx < size)
		w += u_char_width(u_get_char(str, size, &idx));
	return w;
}

unsigned int u_prev_char(const unsigned char *buf, unsigned int *idx)
{
	unsigned int i = *idx;
	unsigned int count, shift;
	unsigned int u;

	u = buf[--i];
	if (likely(u < 0x80)) {
		*idx = i;
		return u;
	}

	if (!u_is_continuation(u))
		goto invalid;

	u &= 0x3f;
	count = 1;
	shift = 6;
	while (i) {
		unsigned int ch = buf[--i];
		unsigned int len = u_seq_len(ch);

		count++;
		if (len == 0) {
			if (count == 4) {
				/* too long sequence */
				break;
			}
			u |= (ch & 0x3f) << shift;
			shift += 6;
		} else if (count != len) {
			/* incorrect length */
			break;
		} else {
			u |= (ch & u_get_first_byte_mask(len)) << shift;
			if (!u_seq_len_ok(u, len))
				break;

			*idx = i;
			return u;
		}
	}
invalid:
	*idx = *idx - 1;
	u = buf[*idx];
	return -u;
}

unsigned int u_get_char(const unsigned char *buf, unsigned int size, unsigned int *idx)
{
	unsigned int i = *idx;
	unsigned int u = buf[i];

	if (likely(u < 0x80)) {
		*idx = i + 1;
		return u;
	}
	return u_get_nonascii(buf, size, idx);
}

unsigned int u_get_nonascii(const unsigned char *buf, unsigned int size, unsigned int *idx)
{
	unsigned int i = *idx;
	int len, c;
	unsigned int first, u;

	first = buf[i++];
	len = u_seq_len(first);
	if (unlikely(len < 2 || len > size - i + 1))
		goto invalid;

	u = first & u_get_first_byte_mask(len);
	c = len - 1;
	do {
		unsigned int ch = buf[i++];
		if (!u_is_continuation(ch))
			goto invalid;
		u = (u << 6) | (ch & 0x3f);
	} while (--c);

	if (!u_seq_len_ok(u, len))
		goto invalid;

	*idx = i;
	return u;
invalid:
	*idx += 1;
	return -first;
}

void u_set_char_raw(char *str, unsigned int *idx, unsigned int uch)
{
	unsigned int i = *idx;

	if (uch <= 0x7fU) {
		str[i++] = uch;
		*idx = i;
	} else if (uch <= 0x7ffU) {
		str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0xc0U;
		i += 2;
		*idx = i;
	} else if (uch <= 0xffffU) {
		str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0xe0U;
		i += 3;
		*idx = i;
	} else if (uch <= 0x10ffffU) {
		str[i + 3] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0xf0U;
		i += 4;
		*idx = i;
	} else {
		/* invalid byte value */
		str[i++] = uch & 0xff;
		*idx = i;
	}
}

void u_set_char(char *str, unsigned int *idx, unsigned int uch)
{
	unsigned int i = *idx;

	if (uch < 0x80) {
		if (likely(!u_is_ctrl(uch))) {
			str[i++] = uch;
			*idx = i;
			return;
		}
		u_set_ctrl(str, idx, uch);
		return;
	}
	if (u_is_unprintable(uch)) {
		u_set_hex(str, idx, uch);
		return;
	}
	if (uch <= 0x7ff) {
		str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0xc0U;
		i += 2;
		*idx = i;
		return;
	}
	if (uch <= 0xffff) {
		str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0xe0U;
		i += 3;
		*idx = i;
		return;
	}
	if (uch <= 0x10ffff) {
		str[i + 3] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0xf0U;
		i += 4;
		*idx = i;
	}
}

void u_set_hex(char *str, unsigned int *idx, unsigned int uch)
{
	unsigned int i = *idx;

	str[i++] = '<';
	if (!u_is_unicode(uch)) {
		// invalid byte (negated)
		uch *= -1;
		str[i++] = hex_tab[(uch >> 4) & 0x0f];
		str[i++] = hex_tab[uch & 0x0f];
	} else {
		str[i++] = '?';
		str[i++] = '?';
	}
	str[i++] = '>';
	*idx = i;
}

unsigned int u_skip_chars(const char *str, int *width)
{
	int w = *width;
	unsigned int idx = 0;

	while (w > 0) {
		unsigned int u = u_get_char(str, idx + 4, &idx);
		w -= u_char_width(u);
	}
	/* add 1..3 if skipped 'too much' (the last char was double width or invalid (<xx>)) */
	*width -= w;
	return idx;
}
