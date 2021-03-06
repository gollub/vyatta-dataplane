/*-
 * Copyright (c) 2017-2018, AT&T Intellectual Property.  All rights reserved.
 * Copyright (c) 2011-2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef IF_ETHER_H
#define IF_ETHER_H

#include <stdint.h>

struct ether_addr;
struct ifnet;
struct llentry;
struct ndmsg;
struct rte_timer;

enum cont_src_en;

void lladdr_update(struct ifnet *ifp, struct llentry *la,
		   const struct ether_addr *enaddr, uint16_t flags);
void lladdr_timer(struct rte_timer *, void *arg);
void in6_lladdr_timer(struct rte_timer *tim, void *arg);
void lladdr_nl_event(int family, struct ifnet *ifp,
			    uint16_t type, const struct ndmsg *ndm,
			    const void *dst,
			    const struct ether_addr *lladdr);
void lladdr_flush_all(enum cont_src_en cont_src);
void ll_addr_set(struct llentry *lle, const struct ether_addr *eth);
/* Call this to link to routes when an lle transitions to VALID */
void llentry_routing_install(struct llentry *lle);

#endif
