/*
 * Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
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

#ifndef PEAK_TRACK_H
#define PEAK_TRACK_H

struct peak_track {
	struct netaddr addr[2];
	uint16_t port[2];
	uint8_t type;
	uint8_t padding[3];
	/* flow tracker info ends here */
	uint16_t li[2];
	uint8_t reserved[4];
	uint64_t id;
	/* glue for tracker managment */
	TAILQ_ENTRY(peak_track) tq_to;
	RB_ENTRY(peak_track) rb_track;
} __packed;

#define TRACK_KEY(flow, packet) do {					\
	const unsigned int dir =					\
	    !(netcmp(&(packet)->net_saddr, &(packet)->net_daddr) < 0);	\
	(flow)->port[dir] = (packet)->flow_sport;			\
	(flow)->port[!dir] = (packet)->flow_dport;			\
	(flow)->addr[dir] = (packet)->net_saddr;			\
	(flow)->addr[!dir] = (packet)->net_daddr;			\
	(flow)->type = (packet)->net_type;				\
} while (0)

struct peak_tracks	*peak_track_init(const size_t);
struct peak_track	*peak_track_acquire(struct peak_tracks *,
			    const struct peak_track *);
void			 peak_track_exit(struct peak_tracks *);

#endif /* !PEAK_TRACK_H */
