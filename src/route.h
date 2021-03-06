/*-
 * Copyright (c) 2017-2019, AT&T Intellectual Property. All rights reserved.
 * Copyright (c) 2011-2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#ifndef ROUTE_H
#define ROUTE_H

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "compiler.h"
#include "control.h"
#include "json_writer.h"
#include "mpls/mpls.h"
#include "pd_show.h"
#include "pktmbuf.h"
#include "route_flags.h"
#include "urcu.h"
#include "util.h"

struct ifnet;
struct rte_mbuf;
struct vrf;
struct ll_entry;

struct route_head {
	uint32_t rt_rtm_max;
	struct lpm **rt_table;
};

#define PBR_TABLEID_MAX 128

static inline bool
tableid_in_pbr_range(uint32_t tableid)
{
	return (tableid > 0 && tableid <= PBR_TABLEID_MAX);
}

/* Output information associated with a single nexthop */
struct next_hop {
	union {
		struct ifnet *ifp;     /* target interface */
		struct llentry *lle;   /* lle entry to use when sending */
	} u;
	uint32_t      flags;   /* routing flags */
	union next_hop_outlabels outlabels;
	in_addr_t     gateway; /* nexthop IP address */
};

/*
 * Nexthop (output information) related APIs
 */
struct next_hop *
nexthop_create(struct ifnet *ifp, in_addr_t gw, uint32_t flags,
	       uint16_t num_labels, label_t *labels);
void nexthop_put(uint32_t idx);
int nexthop_new(const struct next_hop *nh, uint16_t size, uint8_t proto,
		uint32_t *slot);
struct next_hop *nexthop_select(uint32_t nh_idx, const struct rte_mbuf *m,
				uint16_t ether_type);
struct next_hop *nexthop_get(uint32_t nh_idx, uint8_t *size);
void rt_print_nexthop(json_writer_t *json, uint32_t next_hop);

/*
 * IPv4 route table apis.
 */
int route_init(struct vrf *vrf);
void route_uninit(struct vrf *vrf, struct route_head *rt_head);
bool route_link_vrf_to_table(struct vrf *vrf, uint32_t tableid);
bool route_unlink_vrf_from_table(struct vrf *vrf);
struct next_hop *rt_lookup(in_addr_t dst, uint32_t tbl_id,
			   const struct rte_mbuf *m) __hot_func;
struct next_hop *rt_lookup_fast(struct vrf *vrf, in_addr_t dst,
				uint32_t tblid,
				const struct rte_mbuf *m);

int rt_insert(vrfid_t vrf_id, in_addr_t dst, uint8_t depth, uint32_t id,
	      uint8_t scope, uint8_t proto, struct next_hop hops[],
	      size_t size, bool replace);
int rt_delete(vrfid_t vrf_id, in_addr_t dst, uint8_t depth,
	      uint32_t id, uint8_t scope);
void rt_flush_all(enum cont_src_en cont_src);
void rt_flush(struct vrf *vrf);
enum rt_walk_type {
	RT_WALK_LOCAL,
	RT_WALK_RIB,
	RT_WALK_ALL,
};
int rt_walk(struct route_head *rt_head, json_writer_t *json, uint32_t id,
	    uint32_t cnt, enum rt_walk_type type);
int rt_walk_next(struct route_head *rt_head, json_writer_t *json,
		 uint32_t id, const struct in_addr *addr,
		 uint8_t plen, uint32_t cnt, enum rt_walk_type type);
int rt_stats(struct route_head *, json_writer_t *, uint32_t);
void rt_if_handle_in_dataplane(struct ifnet *ifp);
void rt_if_punt_to_slowpath(struct ifnet *ifp);
int rt_show(struct route_head *rt_head, json_writer_t *json, uint32_t tblid,
	    const struct in_addr *addr);
void nexthop_tbl_init(void);
bool rt_valid_tblid(vrfid_t vrfid, uint32_t tblid);
int rt_local_show(struct route_head *rt_head, uint32_t id, FILE *f);
bool is_local_ipv4(vrfid_t vrf_id, in_addr_t dst);

static inline bool
nexthop_is_local(const struct next_hop *nh)
{
	return nh->flags & RTF_LOCAL;
}

struct ifnet *nhif_dst_lookup(const struct vrf *vrf,
			      in_addr_t dst,
			      bool *connected);
int nh_lookup_by_index(uint32_t nhindex, uint32_t hash, in_addr_t *nh,
		       uint32_t *nh_ifindex);

void routing_insert_arp_safe(struct llentry *lle, bool arp_change);
void routing_remove_arp_safe(struct llentry *lle);

uint32_t *route_sw_stats_get(void);
uint32_t *route_hw_stats_get(void);

int route_get_pd_subset_data(json_writer_t *json, enum pd_obj_state subset);

#endif
