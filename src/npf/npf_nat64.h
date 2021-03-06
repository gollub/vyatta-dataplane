/*
 * Copyright (c) 2019, AT&T Intellectual Property.  All rights reserved.
 * Copyright (c) 2016 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef _NPF_NAT64_H_
#define _NPF_NAT64_H_

#include <stdbool.h>
#include <stdint.h>

#include "npf/npf.h"
#include "npf/npf_cache.h"
#include "npf/npf_session.h"

/* Forward Declarations */
struct ifnet;
struct npf_config;
struct rte_mbuf;
struct npf_nat64;
typedef struct npf_rule npf_rule_t;
typedef struct npf_cache npf_cache_t;
typedef struct npf_session npf_session_t;

npf_decision_t
nat64_hook(npf_action_t *action, const struct npf_config *npf_config,
	   npf_session_t **sep, struct ifnet *ifp, npf_cache_t *npc,
	   struct rte_mbuf **m, int dir, uint16_t *npf_flag);

int npf_nat64_session_link(struct npf_session *se1, struct npf_session *se2);
void npf_nat64_session_unlink(struct npf_session *se);
void npf_nat64_session_destroy(struct npf_session *se);
bool npf_nat64_session_is_nat64(npf_session_t *se);
bool npf_nat64_session_is_nat46(npf_session_t *se);
void npf_nat64_session_json(json_writer_t *json, npf_session_t *se);

npf_rule_t *npf_nat64_get_rule(struct npf_nat64 *n64);
bool npf_nat64_has_peer(struct npf_nat64 *n64);
npf_session_t *npf_nat64_get_peer(struct npf_nat64 *n64);
bool npf_nat64_session_log_enabled(struct npf_nat64 *n64);

#endif /* _NPF_NAT64_H_ */
