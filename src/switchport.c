/*-
 * Copyright (c) 2018-2019, AT&T Intellectual Property.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * switchport command handling
 */

#include <stdio.h>
#include "bridge.h"
#include "control.h"
#include "fal.h"
#include "if_var.h"
#include <string.h>
#include "vplane_log.h"
#include <rte_log.h>
#include "dp_event.h"
#include "commands.h"

struct cfg_if_list *cfg_list;

static void
switchport_event_if_index_set(struct ifnet *ifp, uint32_t ifindex);
static void
switchport_event_if_index_unset(struct ifnet *ifp, uint32_t ifindex);

static const struct dp_event_ops switchport_event_ops = {
	.if_index_set = switchport_event_if_index_set,
	.if_index_unset = switchport_event_if_index_unset,
};

static void
switchport_event_if_index_set(struct ifnet *ifp, uint32_t ifindex __unused)
{
	struct cfg_if_list_entry *le;

	if (!cfg_list)
		return;

	le = cfg_if_list_lookup(cfg_list, ifp->if_name);
	if (!le)
		return;

	RTE_LOG(INFO, DATAPLANE,
			"Replaying switchport command %s for interface %s\n",
			le->le_buf, ifp->if_name);
	cmd_switchport(NULL, le->le_argc, le->le_argv);
	cfg_if_list_del(cfg_list, ifp->if_name);
	if (!cfg_list->if_list_count) {
		cfg_if_list_destroy(&cfg_list);
		dp_event_unregister(&switchport_event_ops);
	}
}

static void
switchport_event_if_index_unset(struct ifnet *ifp, uint32_t ifindex __unused)
{
	if (!cfg_list)
		return;

	cfg_if_list_del(cfg_list, ifp->if_name);
	if (!cfg_list->if_list_count) {
		cfg_if_list_destroy(&cfg_list);
		dp_event_unregister(&switchport_event_ops);
	}
}

static int switchport_replay_init(void)
{
	if (!cfg_list) {
		cfg_list = cfg_if_list_create();
		if (!cfg_list)
			return -ENOMEM;
	}
	dp_event_register(&switchport_event_ops);
	return 0;
}

int cmd_switchport(FILE *f, int argc, char **argv)
{
	struct ifnet *ifp;
	struct fal_attribute_t attr;

	if (argc != 4) {
		if (f) {
			fprintf(f, "\nInvalid command : ");
			for (int i = 0; i < argc; i++)
				fprintf(f, "%s ", argv[i]);
			fprintf(f,
					"\nUsage: switchport <ifname> hw-switching <enable|disable>\n");
		}
		return -EINVAL;
	}

	ifp = ifnet_byifname(argv[1]);
	if (!ifp) {
		if (!cfg_list && switchport_replay_init()) {
			RTE_LOG(ERR, DATAPLANE,
					"Could not set up command replay cache\n");
			return -ENOMEM;
		}

		RTE_LOG(INFO, DATAPLANE,
				"Caching switchport command for interface %s\n",
				argv[1]);
		cfg_if_list_add(cfg_list, argv[1], argc, argv);
		return 0;
	}

	if (!strcmp(argv[2], "hw-switching")) {
		if (!strcmp(argv[3], "enable")) {
			attr.value.u8 = FAL_PORT_HW_SWITCHING_ENABLE;
			ifp->hw_forwarding = true;
		} else if (!strcmp(argv[3], "disable")) {
			attr.value.u8 = FAL_PORT_HW_SWITCHING_DISABLE;
			ifp->hw_forwarding = false;
		} else
			return -EINVAL;
		attr.id = FAL_PORT_ATTR_HW_SWITCH_MODE;
		fal_l2_upd_port(ifp->if_index, &attr);
		dp_event(DP_EVT_IF_HW_SWITCHING_CHANGE, 0, ifp,
			 ifp->hw_forwarding, 0, NULL);
		/* TODO move bridge code to using event */
		bridge_upd_hw_forwarding(ifp);
		return 0;
	}

	return -EINVAL;
}
