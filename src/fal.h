 /*-
 * Copyright (c) 2017-2019, AT&T Intellectual Property.
 * All rights reserved.
 * Copyright (c) 2016-2017 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef FAL_H
#define FAL_H
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linux/rtnetlink.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <rte_ether.h>
#include <sys/queue.h>

#include "fal_plugin.h"
#include "if_var.h"
#include "route.h"
#include "netinet/ip_mroute.h"
#include "util.h"
#include "vrf.h"

struct ether_addr;
struct fal_attribute_t;
struct fal_ip_address_t;
struct if_addr;
struct next_hop;
struct next_hop_v6;
struct fal_ipmc_entry_t;

/*
 * Pluggable message handling. Provides handlers for control messages
 * for multiple receiver types.  A 'hdlr' may be ignored in all calls
 * as will be the case for the default 'software dataplane'.
 */

/*
 * When creating a message_handler, all fields must be filled out
 * or set to NULL.
 */
struct message_handler {
	struct l2_ops *l2;
	struct rif_ops *rif;
	struct tun_ops *tun;
	struct bridge_ops *bridge;
	struct vlan_ops *vlan;
	struct stp_ops *stp;
	struct ip_ops *ip;
	struct ipmc_ops *ipmc;
	struct acl_ops *acl;
	struct qos_ops *qos;
	struct lacp_ops *lacp;
	struct sys_ops *sys;
	struct policer_ops *policer;
	struct sw_ops *sw;
	struct mirror_ops *mirror;
	struct vlan_feat_ops *vlan_feat;
	struct backplane_ops *backplane;
	struct cpp_rl_ops *cpp_rl;
	struct ptp_ops *ptp;

	LIST_ENTRY(message_handler) link;
};

/*
 * l2_ops provide an interface for a receiver 'recv' to work with the data
 * parsed from AF_UNSPEC netlink messages.
 */
struct l2_ops {
	void (*new_port)(unsigned int if_index,
			 uint32_t attr_count,
			 const struct fal_attribute_t *attr_list);
	int (*get_attrs)(unsigned int if_index,
			 uint32_t attr_count,
			 struct fal_attribute_t *attr_list);
	void (*upd_port)(unsigned int if_index,
			 struct fal_attribute_t *attr);
	void (*del_port)(unsigned int if_index);
	void (*new_addr)(unsigned int if_index,
			 const void *addr,
			 uint32_t attr_count,
			 const struct fal_attribute_t *attr_list);
	void (*upd_addr)(unsigned int if_index,
			 const void *addr,
			 struct fal_attribute_t *attr);
	void (*del_addr)(unsigned int if_index,
			 const void *addr);
};

/*
 * rif_ops provides an interface for controlling (l3) router intf
 */
struct rif_ops {
	int (*create_intf)(uint32_t attr_count,
			   const struct fal_attribute_t *attr,
			   fal_object_t *obj);
	int (*delete_intf)(fal_object_t obj);
	int (*set_attr)(fal_object_t obj,
			const struct fal_attribute_t *attr);
};

/*
 * tun_ops provides an interface for controlling tunnel initiator
 * and terminator
 */
struct tun_ops {
	int (*create_tun)(uint32_t attr_count,
			  const struct fal_attribute_t *attr,
			  fal_object_t *obj);
	int (*delete_tun)(fal_object_t obj);
	int (*set_attr)(fal_object_t obj, uint32_t attr_count,
			const struct fal_attribute_t *attr);
};

struct stp_ops {
	int (*create)(unsigned int bridge_ifindex, uint32_t attr_count,
		      const struct fal_attribute_t *attr_list,
		      fal_object_t *obj);
	int (*delete)(fal_object_t obj);
	int (*set_attribute)(fal_object_t obj,
			     const struct fal_attribute_t *attr);
	int (*get_attribute)(fal_object_t obj,
			     uint32_t attr_count,
			     struct fal_attribute_t *attr_list);
	int (*set_port_attribute)(unsigned int child_ifindex,
				  uint32_t attr_count,
				  const struct fal_attribute_t *attr_list);
	int (*get_port_attribute)(unsigned int child_ifindex,
				  uint32_t attr_count,
				  struct fal_attribute_t *attr_list);
};

/*
 * bridge_ops provide the ability for a receiver 'hdlr' to work with data
 * parsed from AF_BRIDGE netlink messages.
 */
struct bridge_ops {
	void (*new_port)(unsigned int bridge_ifindex,
			 unsigned int child_ifindex,
			 uint32_t attr_count,
			 const struct fal_attribute_t *attr_list);
	void (*upd_port)(unsigned int child_ifindex,
			 const struct fal_attribute_t *attr_list);
	void (*del_port)(unsigned int bridge_ifindex,
			 unsigned int child_ifindex);
	void (*new_neigh)(unsigned int child_ifindex,
			  uint16_t vlanid,
			  const struct ether_addr *dst,
			  uint32_t attr_count,
			  const struct fal_attribute_t *attr_list);
	void (*upd_neigh)(unsigned int child_ifindex,
			  uint16_t vlanid,
			  const struct ether_addr *dst,
			  struct fal_attribute_t *attr);
	void (*del_neigh)(unsigned int child_ifindex,
			  uint16_t vlanid,
			  const struct ether_addr *dst);
	void (*flush_neigh)(unsigned int bridge_ifindex,
			    uint32_t attr_count,
			    const struct fal_attribute_t *attr_list);
	int (*walk_neigh)(unsigned int bridge_ifindex, uint16_t vlanid,
			  const struct ether_addr *dst,
			  unsigned int child_ifindex,
			  fal_br_walk_neigh_fn cb, void *arg);
};

struct vlan_ops {
	int (*get_stats)(uint16_t vlan, uint32_t num_cntrs,
			 const enum fal_vlan_stat_type *cntr_ids,
			 uint64_t *cntrs);
	int (*clear_stats)(uint16_t vlan, uint32_t num_cntrs,
			   const enum fal_vlan_stat_type *cntr_ids);
};

/*
 * ip_ops provide the ability for a receiver 'hdlr' to work with data
 * parsed from AF_INET netlink messages.
 */
struct ip_ops {
	void (*new_addr)(unsigned int if_index,
			 struct fal_ip_address_t *ipaddr,
			 uint8_t prefixlen,
			 uint32_t attr_count,
			 const struct fal_attribute_t *attr_list);
	void (*upd_addr)(unsigned int if_index,
			 struct fal_ip_address_t *ipaddr,
			 uint8_t prefixlen,
			 struct fal_attribute_t *attr);
	void (*del_addr)(unsigned int if_index,
			 struct fal_ip_address_t *ipaddr,
			 uint8_t prefixlen);
	int (*new_neigh)(unsigned int if_index,
			 struct fal_ip_address_t *ipaddr,
			 uint32_t attr_count,
			 const struct fal_attribute_t *attr_list);
	int (*upd_neigh)(unsigned int if_index,
			 struct fal_ip_address_t *ipaddr,
			 struct fal_attribute_t *attr);
	int (*get_neigh_attrs)(unsigned int if_index,
			       struct fal_ip_address_t *ipaddr,
			       uint32_t attr_count,
			       const struct fal_attribute_t *attr_list);
	int (*del_neigh)(unsigned int if_index,
			 struct fal_ip_address_t *ipaddr);
	int (*new_route)(uint32_t vrf_id,
			 struct fal_ip_address_t *ipaddr,
			 uint8_t prefixlen,
			 uint32_t tableid,
			 uint32_t attr_count,
			 const struct fal_attribute_t *attr_list);
	int (*upd_route)(uint32_t vrf_id,
			 struct fal_ip_address_t *ipaddr,
			 uint8_t prefixlen,
			 uint32_t tableid,
			 struct fal_attribute_t *attr);
	int (*del_route)(uint32_t vrf_id,
			 struct fal_ip_address_t *ipaddr,
			 uint8_t prefixlen,
			 uint32_t tableid);
	int (*new_next_hop_group)(uint32_t attr_count,
				  const struct fal_attribute_t *attr_list,
				  fal_object_t *obj);
	int (*upd_next_hop_group)(fal_object_t obj,
				  const struct fal_attribute_t *attr);
	int (*del_next_hop_group)(fal_object_t obj);
	int (*new_next_hops)(uint32_t nh_count,
			     const uint32_t *attr_count,
			     const struct fal_attribute_t **attr_list,
			     fal_object_t *obj_list);
	int (*upd_next_hop)(fal_object_t obj,
			    const struct fal_attribute_t *attr);
	int (*del_next_hops)(uint32_t nh_count,
			     const fal_object_t *obj_list);
};

struct acl_ops {
	/* A "table" corresponds to a named "group" */
	int (*create_table)(uint32_t attr_count,
			    const struct fal_attribute_t *attr,
			    fal_object_t *new_table_id);
	int (*delete_table)(fal_object_t table_id);
	int (*set_table_attr)(fal_object_t table_id,
			      const struct fal_attribute_t *attr);
	int (*get_table_attr)(fal_object_t table_id,
			      uint32_t attr_count,
			      struct fal_attribute_t *attr_list);
	/* An "entry" corresponds to a numbered "rule" in a "group" */
	int (*create_entry)(uint32_t attr_count,
			    const struct fal_attribute_t *attr,
			    fal_object_t *new_entry_id);
	int (*delete_entry)(fal_object_t entry_id);
	int (*set_entry_attr)(fal_object_t entry_id,
			      const struct fal_attribute_t *attr);
	int (*get_entry_attr)(fal_object_t entry_id,
			      uint32_t attr_count,
			      struct fal_attribute_t *attr_list);
	/* A "counter" is associated with a "table" (aka named "group") */
	int (*create_counter)(uint32_t attr_count,
			      const struct fal_attribute_t *attr,
			      fal_object_t *new_counter_id);
	int (*delete_counter)(fal_object_t counter_id);
	int (*set_counter_attr)(fal_object_t counter_id,
				const struct fal_attribute_t *attr);
	int (*get_counter_attr)(fal_object_t counter_id,
				uint32_t attr_count,
				struct fal_attribute_t *attr_list);
	/*
	 * We will eventually add "table_group" and "table_group_member" which
	 * correspond to "rulesets" of named "groups" on an "attach point".
	 *
	 * We may eventually add "range".
	 */
};

/*
 * ipmc_ops provide the ability for a receiver 'hdlr' to work with data
 * parsed from AF_INET multicast netlink messages.
 */
struct ipmc_ops {
	int (*create_entry)(const struct fal_ipmc_entry_t *ipmc_entry,
			    uint32_t attr_count,
			    const struct fal_attribute_t *attr_list,
			    fal_object_t *obj);
	int (*delete_entry)(fal_object_t obj);
	int (*set_entry_attr)(fal_object_t obj,
			      const struct fal_attribute_t *attr);
	int (*get_entry_attr)(fal_object_t obj,
			      uint32_t attr_count,
			      const struct fal_attribute_t *attr_list);
	int (*get_entry_stats)(
		fal_object_t obj,
		uint32_t num_counters,
		const enum fal_ip_mcast_entry_stat_type *cntr_ids,
		uint64_t *cntrs);
	int (*clear_entry_stats)(
		fal_object_t obj,
		uint32_t num_counters,
		const enum fal_ip_mcast_entry_stat_type *cntr_ids);
	int (*create_group)(uint32_t attr_count,
			    const struct fal_attribute_t *attr_list,
			    fal_object_t *obj);
	int (*delete_group)(fal_object_t obj);
	int (*set_group_attr)(fal_object_t obj,
			      const struct fal_attribute_t *attr);
	int (*get_group_attr)(fal_object_t obj,
			      uint32_t attr_count,
			      const struct fal_attribute_t *attr_list);
	int (*create_member)(uint32_t attr_count,
			     const struct fal_attribute_t *attr_list,
			     fal_object_t *obj);
	int (*delete_member)(fal_object_t obj);
	int (*set_member_attr)(fal_object_t obj,
			       const struct fal_attribute_t *attr);
	int (*get_member_attr)(fal_object_t obj,
			       uint32_t attr_count,
			       const struct fal_attribute_t *attr_list);
	int (*create_rpf_group)(uint32_t attr_count,
				const struct fal_attribute_t *attr_list,
				fal_object_t *obj);
	int (*delete_rpf_group)(fal_object_t obj);
	int (*set_rpf_group_attr)(fal_object_t obj,
				  const struct fal_attribute_t *attr);
	int (*get_rpf_group_attr)(fal_object_t obj,
				  uint32_t attr_count,
				  const struct fal_attribute_t *attr_list);
	int (*create_rpf_member)(uint32_t attr_count,
				 const struct fal_attribute_t *attr_list,
				 fal_object_t *obj);
	int (*delete_rpf_member)(fal_object_t obj);
	int (*set_rpf_member_attr)(fal_object_t obj,
				   const struct fal_attribute_t *attr);
	int (*get_rpf_member_attr)(fal_object_t obj,
				   uint32_t attr_count,
				   const struct fal_attribute_t *attr_list);
};

/* qos_ops provide ability handle vyatta-dataplane QoS configuration commands */
struct qos_ops {
	/* QoS queue object functions */
	int (*new_queue)(fal_object_t switch_id,
			 uint32_t attr_count,
			 const struct fal_attribute_t *attr_list,
			 fal_object_t *new_queue_id);
	int (*del_queue)(fal_object_t queue_id);
	int (*upd_queue)(fal_object_t queue_id,
			 const struct fal_attribute_t *attr);
	int (*get_queue_attrs)(fal_object_t queue_id,
			       uint32_t attr_count,
			       struct fal_attribute_t *attr_list);
	int (*get_queue_stats)(fal_object_t queue_id,
			       uint32_t number_of_counters,
			       const uint32_t *counter_ids, uint64_t *counters);
	int (*get_queue_stats_ext)(fal_object_t queue_id,
				   uint32_t number_of_counters,
				   const uint32_t *counter_ids,
				   bool read_and_clear, uint64_t *counters);
	int (*clear_queue_stats)(fal_object_t queue_id,
				 uint32_t number_of_counters,
				 const uint32_t *counter_ids);
	/* QoS map object functions */
	int (*new_map)(fal_object_t switch_id,
		       uint32_t attr_count,
		       const struct fal_attribute_t *attr_list,
		       fal_object_t *new_map_id);
	int (*del_map)(fal_object_t map_id);
	int (*upd_map)(fal_object_t map_id,
		       const struct fal_attribute_t *attr);
	int (*get_map_attrs)(fal_object_t map_id,
			     uint32_t attr_count,
			     struct fal_attribute_t *attr_list);
	void (*dump_map)(fal_object_t map_id, json_writer_t *wr);
	/* QoS scheduler object functions */
	int (*new_scheduler)(fal_object_t switch_id,
			     uint32_t attr_count,
			     const struct fal_attribute_t *attr_list,
			     fal_object_t *new_scheduler_id);
	int (*del_scheduler)(fal_object_t scheduler_id);
	int (*upd_scheduler)(fal_object_t scheduler_id,
			     const struct fal_attribute_t *attr);
	int (*get_scheduler_attrs)(fal_object_t scheduler_id,
				   uint32_t attr_count,
				   struct fal_attribute_t *attr_list);
	/* QoS scheduler-group object functions */
	int (*new_sched_group)(fal_object_t switch_id,
			       uint32_t attr_count,
			       const struct fal_attribute_t *attr_list,
			       fal_object_t *new_sched_group_id);
	int (*del_sched_group)(fal_object_t sched_group_id);
	int (*upd_sched_group)(fal_object_t sched_group_id,
			       const struct fal_attribute_t *attr);
	int (*get_sched_group_attrs)(fal_object_t sched_group_id,
				     uint32_t attr_count,
				     struct fal_attribute_t *attr_list);
	void (*dump_sched_group)(fal_object_t sg_id, json_writer_t *wr);
	/* QoS WRED object functions */
	int (*new_wred)(fal_object_t switch_id,
			uint32_t attr_count,
			const struct fal_attribute_t *attr_list,
			fal_object_t *new_wred_id);
	int (*del_wred)(fal_object_t wred_id);
	int (*upd_wred)(fal_object_t wred_id,
			const struct fal_attribute_t *attr);
	int (*get_wred_attrs)(fal_object_t wred_id, uint32_t attr_count,
			      struct fal_attribute_t *attr_list);
};

struct sw_ops {
	int (*set_attribute)(const struct fal_attribute_t *attr);
	int (*get_attribute)(uint32_t attr_count,
			     struct fal_attribute_t *attr_list);
};

/* sys_ops provide ability to handle system level events */
struct sys_ops {
	void (*cleanup)(void);
	void (*command)(FILE *f, int argc, char **argv);
	int (*command_ret)(FILE *f, int argc, char **argv);
};

/*
 * policer ops are used for setting up storm control and
 * other traffic policing operations.
 */
struct policer_ops {
	/* The policer APIs follow SAI approach */
	int (*create)(uint32_t attr_count,
		      const struct fal_attribute_t *attr_list,
		      fal_object_t *obj);
	int (*delete)(fal_object_t obj);
	int (*set_attr)(fal_object_t obj,
			const struct fal_attribute_t *attr);
	int (*get_attr)(fal_object_t obj,
			uint32_t attr_count,
			struct fal_attribute_t *attr_list);
	int (*get_stats_ext)(fal_object_t obj,
			     uint32_t num_counters,
			     const enum fal_policer_stat_type *cntr_ids,
			     enum fal_stats_mode mode,
			     uint64_t *stats);
	int (*clear_stats)(fal_object_t obj,
			   uint32_t num_counters,
			   const enum fal_policer_stat_type *cntr_ids);
	void (*dump)(fal_object_t obj, json_writer_t *wr);
};

/**
 * Portmirror/portmonitor operations used for setting,updating and
 * deleting portmonitor session
 */
struct mirror_ops {
	int (*session_create)(uint32_t attr_count,
			      const struct fal_attribute_t *attr_list,
			      fal_object_t *mr_obj_id);
	int (*session_delete)(fal_object_t mr_obj_id);
	int (*session_set_attr)(fal_object_t mr_obj_id,
				const struct fal_attribute_t *attr);
	int (*session_get_attr)(fal_object_t mr_obj,
				uint32_t attr_count,
				struct fal_attribute_t *attr_list);
};

/**
 * Vlan_feature operations user for setting, updating and creating a vlan
 * feature.
 */
struct vlan_feat_ops {
	int (*vlan_feature_create)(uint32_t attr_count,
				   const struct fal_attribute_t *attr_list,
				   fal_object_t *fal_obj_id);
	int (*vlan_feature_delete)(fal_object_t fal_obj_id);
	int (*vlan_feature_set_attr)(fal_object_t fal_obj_id,
				     const struct fal_attribute_t *attr);
};

struct backplane_ops {
	int (*backplane_bind)(unsigned int bp_ifindex, unsigned int ifindex);
	void (*backplane_dump)(unsigned int bp_ifindex, json_writer_t *wr);
};

/*
 * cpp_rl_ops are used for setting up control plane policing rate limiter
 * operations
 */
struct cpp_rl_ops {
	/* CPP rate limiter object functions */
	int (*create)(uint32_t attr_count,
		      const struct fal_attribute_t *attr_list,
		      fal_object_t *new_limiter_id);
	int (*remove)(fal_object_t limiter_id);
	int (*get_attrs)(fal_object_t limiter_id, uint32_t attr_count,
			 struct fal_attribute_t *attr_list);
};

struct ptp_ops {
	int (*create_ptp_clock)(uint32_t attr_count,
				const struct fal_attribute_t *attr_list,
				fal_object_t *clock_obj);
	int (*delete_ptp_clock)(fal_object_t clock_obj);
	int (*dump_ptp_clock)(fal_object_t clock_obj,
			      json_writer_t *wr);
	int (*create_ptp_port)(uint32_t attr_count,
			       const struct fal_attribute_t *attr_list,
			       fal_object_t *port_obj);
	int (*delete_ptp_port)(fal_object_t port_obj);
	int (*create_ptp_peer)(uint32_t attr_count,
			       const struct fal_attribute_t *attr_list,
			       fal_object_t *peer_obj);
	int (*delete_ptp_peer)(fal_object_t peer_obj);
};

void fal_init(void);
void fal_init_plugins(void);
void fal_cleanup(void);
int  cmd_fal(FILE *f, int argc, char **argv);
bool fal_plugins_present(void);
int str_to_fal_ip_address_t(char *str, struct fal_ip_address_t *ipaddr);
const char *fal_ip_address_t_to_str(struct fal_ip_address_t *ipaddr,
				    char *dst, socklen_t size);

void fal_register_message_handler(struct message_handler *handler);
void fal_delete_message_handler(struct message_handler *handler);

/* Set the ip addr into the given attr */
void fal_attr_set_ip_addr(struct fal_attribute_t *attr, struct ip_addr *ip);

void fal_l2_new_port(unsigned int if_index,
		     uint32_t attr_count,
		     const struct fal_attribute_t *attr_list);
int fal_l2_get_attrs(unsigned int if_index,
		     uint32_t attr_count,
		     struct fal_attribute_t *attr_list);
void fal_l2_upd_port(unsigned int if_index,
		     struct fal_attribute_t *attr);
void fal_l2_del_port(unsigned int if_index);
void fal_l2_new_addr(unsigned int if_index,
		     const struct ether_addr *addr,
		     uint32_t attr_count,
		     const struct fal_attribute_t *attr_list);
void fal_l2_upd_addr(unsigned int if_index,
		     const struct ether_addr *addr,
		     struct fal_attribute_t *attr);
void fal_l2_del_addr(unsigned int if_index,
		     const struct ether_addr *addr);

/* Router Interface related APIs */
int fal_create_router_interface(uint32_t attr_count,
				struct fal_attribute_t *attr_list,
				fal_object_t *obj);
int fal_delete_router_interface(fal_object_t obj);
int fal_set_router_interface_attr(fal_object_t obj,
				  const struct fal_attribute_t *attr);

/* Tunnel APIs */
int fal_create_tunnel(uint32_t attr_count,
		      struct fal_attribute_t *attr_list,
		      fal_object_t *obj);
int fal_delete_tunnel(fal_object_t obj);
int fal_set_tunnel_attr(fal_object_t obj,
			uint32_t attr_count,
			const struct fal_attribute_t *attr_list);

void fal_br_new_port(unsigned int bridge_ifindex,
		     unsigned int child_ifindex,
		     uint32_t attr_count,
		     const struct fal_attribute_t *attr_list);
void fal_br_upd_port(unsigned int child_ifindex,
		     struct fal_attribute_t *attr);
void fal_br_del_port(unsigned int bridge_ifindex,
		     unsigned int child_ifindex);
void fal_br_new_neigh(unsigned int child_ifindex,
		      uint16_t vlanid,
		      const struct ether_addr *dst,
		      uint32_t attr_count,
		      const struct fal_attribute_t *attr_list);
void fal_br_upd_neigh(unsigned int child_ifindex,
		      uint16_t vlanid,
		      const struct ether_addr *dst,
		      struct fal_attribute_t *attr);
void fal_br_del_neigh(unsigned int child_ifindex,
		      uint16_t vlanid,
		      const struct ether_addr *dst);
void fal_br_flush_neigh(unsigned int bridge_ifindex,
			uint32_t attr_count,
			const struct fal_attribute_t *attr);
void fal_fdb_flush_mac(unsigned int bridge_ifindex, unsigned int child_ifindex,
		       const struct ether_addr *mac);
void fal_fdb_flush(unsigned int bridge_ifindex, unsigned int child_ifindex,
		   uint16_t vlanid, bool only_dynamic);
int fal_br_walk_neigh(unsigned int bridge_ifindex, uint16_t vlanid,
		      const struct ether_addr *dst, unsigned int child_ifindex,
		      fal_br_walk_neigh_fn cb, void *arg);

int fal_vlan_get_stats(uint16_t vlan, uint32_t num_cntrs,
		       const enum fal_vlan_stat_type *cntr_ids,
		       uint64_t *cntrs);
int fal_vlan_clear_stats(uint16_t vlan, uint32_t num_cntrs,
			 const enum fal_vlan_stat_type *cntr_ids);

int fal_stp_create(unsigned int bridge_ifindex, uint32_t attr_count,
		   const struct fal_attribute_t *attr_list,
		   fal_object_t *obj);
int fal_stp_delete(fal_object_t obj);
int fal_stp_set_attribute(fal_object_t obj,
			  const struct fal_attribute_t *attr);
int fal_stp_get_attribute(fal_object_t obj, uint32_t attr_count,
			  struct fal_attribute_t *attr_list);
int fal_stp_set_port_attribute(unsigned int child_ifindex,
			       uint32_t attr_count,
			       const struct fal_attribute_t *attr_list);
int fal_stp_get_port_attribute(unsigned int child_ifindex,
			       uint32_t attr_count,
			       struct fal_attribute_t *attr_list);
int fal_stp_upd_msti(fal_object_t obj, int vlancount, const uint16_t *vlans);
int fal_stp_upd_hw_forwarding(fal_object_t obj, unsigned int if_index,
			      bool hw_forwarding);

int fal_get_switch_attrs(uint32_t attr_count,
			 struct fal_attribute_t *attr_list);

int fal_set_switch_attr(const struct fal_attribute_t *attr);

int fal_ip_upd_neigh(unsigned int if_index,
		     const struct sockaddr *sa,
		     const struct fal_attribute_t *attr);
int fal_ip_get_neigh_attrs(unsigned int if_index,
			   const struct sockaddr *sa,
			   uint32_t attr_count,
			   struct fal_attribute_t *attr_list);

int fal_ip4_new_neigh(unsigned int if_index,
		      const struct sockaddr_in *sin,
		      uint32_t attr_count,
		      const struct fal_attribute_t *attr_list);
int fal_ip4_upd_neigh(unsigned int if_index,
		      const struct sockaddr_in *sin,
		      struct fal_attribute_t *attr);
int fal_ip4_del_neigh(unsigned int if_index,
		      const struct sockaddr_in *sin);
void fal_ip4_new_addr(unsigned int if_index,
		      const struct if_addr *ifa);
void fal_ip4_upd_addr(unsigned int if_index,
		      const struct if_addr *ifa);
void fal_ip4_del_addr(unsigned int if_index,
		      const struct if_addr *ifa);
int fal_ip4_new_next_hops(size_t nhops, const struct next_hop hops[],
			  fal_object_t *nhg_object, fal_object_t *obj);
int fal_ip4_del_next_hops(fal_object_t nhg_object, size_t nhops,
			  const struct next_hop *hops,
			  const fal_object_t *obj);
int fal_ip4_new_route(vrfid_t vrf_id, in_addr_t addr, uint8_t prefixlen,
		      uint32_t tableid, struct next_hop hops[],
		      size_t size, fal_object_t nhg_object);
int fal_ip4_upd_route(vrfid_t vrf_id, in_addr_t addr, uint8_t prefixlen,
		      uint32_t tableid, struct next_hop hops[],
		      size_t size, fal_object_t nhg_object);
int fal_ip4_del_route(vrfid_t vrf_id, in_addr_t addr, uint8_t prefixlen,
		      uint32_t tableid);
int fal_create_ipmc_rpf_group(uint32_t *ifindex_list, uint32_t num_int,
			      fal_object_t *rpf_group_id,
			      struct fal_object_list_t **rpf_member_list);
void fal_cleanup_ipmc_rpf_group(fal_object_t *rpf_group_id,
				struct fal_object_list_t
				**rpf_member_list);
int fal_ip4_new_mroute(vrfid_t vrf_id, struct vmfcctl *mfc, struct mfc *rt,
		       struct cds_lfht *iftable);
int fal_ip4_del_mroute(struct mfc *rt);
int fal_ip4_upd_mroute(fal_object_t obj, struct mfc *rt, struct vmfcctl *mfc,
			struct cds_lfht *iftable);
int fal_ip6_new_mroute(vrfid_t vrf_id, struct vmf6cctl *mfc, struct mf6c *rt,
		       struct cds_lfht *iftable);
int fal_ip6_del_mroute(struct mf6c *rt);
int fal_ip6_upd_mroute(fal_object_t obj, struct mf6c *rt, struct vmf6cctl *mfc,
			struct cds_lfht *iftable);

int fal_ip6_new_neigh(unsigned int if_index,
		      const struct sockaddr_in6 *sin6,
		      uint32_t attr_count,
		      const struct fal_attribute_t *attr_list);
int fal_ip6_upd_neigh(unsigned int if_index,
		      const struct sockaddr_in6 *sin6,
		      struct fal_attribute_t *attr);
int fal_ip6_del_neigh(unsigned int if_index,
		      const struct sockaddr_in6 *sin6);
void fal_ip6_new_addr(unsigned int if_index,
		      const struct if_addr *ifa);
void fal_ip6_upd_addr(unsigned int if_index,
		      const struct if_addr *ifa);
void fal_ip6_del_addr(unsigned int if_index,
		      const struct if_addr *ifa);
int fal_ip6_new_next_hops(size_t nhops, const struct next_hop_v6 hops[],
			  fal_object_t *group_obj, fal_object_t *obj);
int fal_ip6_del_next_hops(fal_object_t group_obj, size_t nhops,
			  const struct next_hop_v6 *hops,
			  const fal_object_t *obj);
int fal_ip6_new_route(vrfid_t vrf_id, const struct in6_addr *addr,
		      uint8_t prefixlen, uint32_t tableid,
		      struct next_hop_v6 hops[], size_t size,
		      fal_object_t group_obj);
int fal_ip6_upd_route(vrfid_t vrf_id, const struct in6_addr *addr,
		      uint8_t prefixlen, uint32_t tableid,
		      struct next_hop_v6 hops[], size_t size,
		      fal_object_t group_obj);
int fal_ip6_del_route(vrfid_t vrf_id, const struct in6_addr *addr,
		      uint8_t prefixlen, uint32_t tableid);

int fal_ip_mcast_get_stats(fal_object_t obj, uint32_t num_counters,
			   const enum fal_ip_mcast_entry_stat_type *cntr_ids,
			   uint64_t *cntrs);
int fal_create_ip_mcast_entry(const struct fal_ipmc_entry_t *ipmc_entry,
			      uint32_t attr_count,
			      const struct fal_attribute_t *attr_list,
			      fal_object_t *obj);
int fal_delete_ip_mcast_entry(fal_object_t obj);
int fal_set_ip_mcast_entry_attr(fal_object_t obj,
				const struct fal_attribute_t *attr);
int fal_get_ip_mcast_entry_attr(fal_object_t obj,
				uint32_t attr_count,
				const struct fal_attribute_t *attr_list);

int fal_create_ip_mcast_group(uint32_t attr_count,
			      const struct fal_attribute_t *attr_list,
			      fal_object_t *obj);
int fal_delete_ip_mcast_group(fal_object_t obj);
int fal_set_ip_mcast_group_attr(fal_object_t obj,
				const struct fal_attribute_t *attr);
int fal_get_ip_mcast_group_attr(fal_object_t obj,
				uint32_t attr_count,
				const struct fal_attribute_t *attr_list);

int fal_create_ip_mcast_group_member(uint32_t attr_count,
				     const struct fal_attribute_t *attr_list,
				     fal_object_t *obj);
int fal_delete_ip_mcast_group_member(fal_object_t obj);
int fal_set_ip_mcast_group_member_attr(fal_object_t obj,
				       const struct fal_attribute_t *attr);
int fal_get_ip_mcast_group_member_attr(fal_object_t obj,
				       uint32_t attr_count,
				       const struct fal_attribute_t *attr_list);

int fal_create_rpf_group(uint32_t attr_count,
			 const struct fal_attribute_t *attr_list,
			 fal_object_t *obj);
int fal_delete_rpf_group(fal_object_t obj);
int fal_set_rpf_group_attr(fal_object_t obj,
			   const struct fal_attribute_t *attr);
int fal_get_rpf_group_attr(fal_object_t obj,
			   uint32_t attr_count,
			   const struct fal_attribute_t *attr_list);

int fal_create_rpf_group_member(uint32_t attr_count,
				const struct fal_attribute_t *attr_list,
				fal_object_t *obj);
int fal_delete_rpf_group_member(fal_object_t obj);
int fal_set_rpf_group_member_attr(fal_object_t obj,
				  const struct fal_attribute_t *attr);
int fal_get_rpf_group_member_attr(fal_object_t obj,
				  uint32_t attr_count,
				  const struct fal_attribute_t *attr_list);

const char *fal_traffic_type_to_str(enum fal_traffic_type tr_type);
enum fal_traffic_type fal_traffic_str_to_type(const char *str);

int fal_policer_create(uint32_t attr_count,
		       const struct fal_attribute_t *attr_list,
		       fal_object_t *obj);
int fal_policer_delete(fal_object_t obj);
int fal_policer_set_attr(fal_object_t obj, const struct fal_attribute_t *attr);
int fal_policer_get_attr(fal_object_t obj, uint32_t attr_count,
			 struct fal_attribute_t *attr_list);
int fal_policer_get_stats_ext(fal_object_t obj, uint32_t num_counters,
			      const enum fal_policer_stat_type *cntr_ids,
			      enum fal_stats_mode mode, uint64_t *stats);
int fal_policer_clear_stats(fal_object_t obj,
			    uint32_t num_counters,
			    const enum fal_policer_stat_type *cntr_ids);
void fal_policer_dump(fal_object_t obj, json_writer_t *wr);

/* QoS function prototypes */
int fal_qos_new_queue(fal_object_t switch_id, uint32_t attr_count,
		      const struct fal_attribute_t *attr_list,
		      fal_object_t *new_queue_id);
int fal_qos_del_queue(fal_object_t queue_id);
int fal_qos_upd_queue(fal_object_t queue_id,
		      const struct fal_attribute_t *attr);
int fal_qos_get_queue_stats(fal_object_t queue_id, uint32_t number_of_counters,
			    const uint32_t *counter_ids,
			    uint64_t *counters);
int fal_qos_get_queue_stats_ext(fal_object_t queue_id,
				uint32_t number_of_counters,
				const uint32_t *counter_ids,
				bool read_and_clear, uint64_t *counters);
int fal_qos_clear_queue_stats(fal_object_t queue_id,
			      uint32_t number_of_counters,
			      const uint32_t *counter_ids);
int fal_qos_get_queue_attrs(fal_object_t queue_id, uint32_t attr_count,
			    struct fal_attribute_t *attr_list);
int fal_qos_new_map(fal_object_t switch_id, uint32_t attr_count,
		    const struct fal_attribute_t *attr_list,
		    fal_object_t *new_map_id);
int fal_qos_del_map(fal_object_t map_id);
int fal_qos_upd_map(fal_object_t map_id, const struct fal_attribute_t *attr);
int fal_qos_get_map_attrs(fal_object_t map_id, uint32_t attr_count,
			 struct fal_attribute_t *attr_list);
int fal_qos_new_scheduler(fal_object_t switch_id, uint32_t attr_count,
			  const struct fal_attribute_t *attr_list,
			  fal_object_t *new_scheduler_id);
int fal_qos_del_scheduler(fal_object_t scheduler_id);
int fal_qos_upd_scheduler(fal_object_t scheduler_id,
			  const struct fal_attribute_t *attr);
int fal_qos_get_scheduler_attrs(fal_object_t scheduler_id, uint32_t attr_count,
			       struct fal_attribute_t *attr_list);
int fal_qos_new_sched_group(fal_object_t switch_id, uint32_t attr_count,
			    const struct fal_attribute_t *attr_list,
			    fal_object_t *new_sched_group_id);
int fal_qos_del_sched_group(fal_object_t sched_group_id);
int fal_qos_upd_sched_group(fal_object_t sched_group_id,
			    const struct fal_attribute_t *attr);
int fal_qos_get_sched_group_attrs(fal_object_t sched_group_id,
				  uint32_t attr_count,
				  struct fal_attribute_t *attr_list);
int fal_qos_new_wred(fal_object_t switch_id, uint32_t attr_count,
		     const struct fal_attribute_t *attr_list,
		     fal_object_t *new_wred_id);
int fal_qos_del_wred(fal_object_t wred_id);
int fal_qos_upd_wred(fal_object_t wred_id,  const struct fal_attribute_t *attr);
int fal_qos_get_wred_attrs(fal_object_t wred_id, uint32_t attr_count,
			  struct fal_attribute_t *attr_list);
void fal_qos_dump_map(fal_object_t obj, json_writer_t *wr);
void fal_qos_dump_sched_group(fal_object_t obj, json_writer_t *wr);

int fal_mirror_session_create(uint32_t attr_count,
			      const struct fal_attribute_t *attr_list,
			      fal_object_t *mr_obj_id);
int fal_mirror_session_delete(fal_object_t mr_obj_id);
int fal_mirror_session_set_attr(fal_object_t mr_obj_id,
				const struct fal_attribute_t *attr);
int fal_mirror_session_get_attr(fal_object_t mr_obj, uint32_t attr_count,
				 struct fal_attribute_t *attr_list);

/* Feature storage id for features to access per packet feature data */
uint8_t fal_feat_storageid(void);

int fal_vlan_feature_create(uint32_t attr_count,
			    const struct fal_attribute_t *attr_list,
			    fal_object_t *fal_obj_id);
int fal_vlan_feature_delete(fal_object_t fal_obj_id);
int fal_vlan_feature_set_attr(fal_object_t fal_obj_id,
			      const struct fal_attribute_t *attr);

int fal_backplane_bind(unsigned int bp_ifindex, unsigned int ifindex);
void fal_backplane_dump(unsigned int bp_ifindex, json_writer_t *wr);

int fal_create_cpp_limiter(uint32_t attr_count,
			   const struct fal_attribute_t *attr_list,
			   fal_object_t *new_limiter_id);
int fal_remove_cpp_limiter(fal_object_t limiter_id);
int fal_get_cpp_limiter_attribute(fal_object_t limiter_id, uint32_t attr_count,
				  struct fal_attribute_t *attr_list);

int fal_create_ptp_clock(uint32_t attr_count,
			 const struct fal_attribute_t *attr_list,
			 fal_object_t *clock_obj);
int fal_delete_ptp_clock(fal_object_t clock_obj);
int fal_dump_ptp_clock(fal_object_t clock_obj, json_writer_t *wr);
int fal_create_ptp_port(uint32_t attr_count,
			const struct fal_attribute_t *attr_list,
			fal_object_t *port);
int fal_delete_ptp_port(fal_object_t port);
int fal_create_ptp_peer(uint32_t attr_count,
			const struct fal_attribute_t *attr_list,
			fal_object_t *peer);
int fal_delete_ptp_peer(fal_object_t peer);

/* The various ACL related functions */
int fal_acl_create_table(uint32_t attr_count,
			 const struct fal_attribute_t *attr,
			 fal_object_t *new_table_id);
int fal_acl_delete_table(fal_object_t table_id);
int fal_acl_set_table_attr(fal_object_t table_id,
			   const struct fal_attribute_t *attr);
int fal_acl_get_table_attr(fal_object_t table_id,
			   uint32_t attr_count,
			   struct fal_attribute_t *attr_list);
int fal_acl_create_entry(uint32_t attr_count,
			 const struct fal_attribute_t *attr,
			 fal_object_t *new_entry_id);
int fal_acl_delete_entry(fal_object_t entry_id);
int fal_acl_set_entry_attr(fal_object_t entry_id,
			   const struct fal_attribute_t *attr);
int fal_acl_get_entry_attr(fal_object_t entry_id,
			   uint32_t attr_count,
			   struct fal_attribute_t *attr_list);
int fal_acl_create_counter(uint32_t attr_count,
			   const struct fal_attribute_t *attr,
			   fal_object_t *new_counter_id);
int fal_acl_delete_counter(fal_object_t counter_id);
int fal_acl_set_counter_attr(fal_object_t counter_id,
			     const struct fal_attribute_t *attr);
int fal_acl_get_counter_attr(fal_object_t counter_id,
			     uint32_t attr_count,
			     struct fal_attribute_t *attr_list);
/* End of ACL related functions */

#endif /* FAL_H */
