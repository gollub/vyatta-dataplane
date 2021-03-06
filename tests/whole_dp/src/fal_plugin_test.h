#ifndef FAL_PLUGIN_TEST_H
#define FAL_PLUGIN_TEST_H
#include <stdbool.h>

extern bool dp_test_fal_plugin_called;

struct fal_policer {
	uint32_t meter;  /* always packets */
	uint32_t mode;   /* always storm ctl */
	uint32_t rate;   /* always in bps, irrespective of user cfg.*/
	uint32_t action; /* Always drop */
	uint32_t burst;  /* always 1 */
	bool assert_transitions;
};

struct vlan_feat {
	int      ifindex;
	uint16_t vlan;
	struct fal_policer *policer[FAL_TRAFFIC_MAX];
};

#endif /* FAL_PLUGIN_TEST_H */
