/*
 * Copyright (c) 2012 Franco Fichtner <franco@lastsummer.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PEAK_LOAD_H
#define PEAK_LOAD_H

struct peak_load {
	unsigned int fmt;
	unsigned int len;
	unsigned int ll;
	int fd;

	uint32_t ts_off;
	uint32_t ts_ms;
	time_t ts_unix;

	uint8_t buf[32 * 1024];
};

void			 peak_load_exit(struct peak_load *);
unsigned int		 peak_load_next(struct peak_load *);
struct peak_load	*peak_load_init(const char *);

#endif /* !PEAK_LOAD_H */