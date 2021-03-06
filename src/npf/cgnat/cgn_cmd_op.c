/*
 * Copyright (c) 2019, AT&T Intellectual Property.  All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @file cgn_cmd_op.c - CGNAT op-mode
 */

#include <errno.h>
#include <netinet/in.h>
#include <linux/if.h>

#include "commands.h"
#include "compiler.h"
#include "config.h"
#include "if_var.h"
#include "util.h"
#include "vplane_log.h"
#include "dp_event.h"

#include "npf/npf_addr.h"

#include "npf/nat/nat_pool_public.h"

#include "npf/cgnat/cgn.h"
#include "npf/apm/apm.h"
#include "npf/cgnat/cgn_errno.h"
#include "npf/cgnat/cgn_if.h"
#include "npf/cgnat/cgn_policy.h"
#include "npf/cgnat/cgn_session.h"
#include "npf/cgnat/cgn_source.h"


static void cgn_show_summary(FILE *f, int argc __unused, char **argv __unused)
{
	json_writer_t *json;

	json = jsonw_new(f);
	if (!json)
		return;

	jsonw_name(json, "summary");
	jsonw_start_object(json);

	cgn_policy_jsonw_summary(json);

	jsonw_uint_field(json, "sess_count",
			 rte_atomic32_read(&cgn_sessions_used));
	jsonw_uint_field(json, "sess2_count",
			 rte_atomic32_read(&cgn_sess2_used));
	jsonw_uint_field(json, "max_sess", cgn_sessions_max);
	jsonw_bool_field(json, "sess_table_full", cgn_session_table_full);

	jsonw_uint_field(json, "subs_table_used", cgn_source_get_used());
	jsonw_uint_field(json, "subs_table_max", cgn_source_get_max());

	jsonw_uint_field(json, "apm_table_used", apm_get_used());
	jsonw_uint_field(json, "apm_table_max", apm_get_max());

	jsonw_uint_field(json, "pkts_hairpinned",
			 rte_atomic64_read(&cgn_hairpinned_pkts));

	/*
	 * Also summarize select error counts.  Mosts counts will only ever
	 * increment in the outbound direction since that is when we are
	 * allocating resources.
	 */
	uint64_t count;

	count = rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_PCY_ENOENT]);
	jsonw_uint_field(json, "nopolicy", count);

	count = rte_atomic64_read(&cgn_errors[CGN_DIR_IN][CGN_SESS_ENOENT]);
	jsonw_uint_field(json, "nosess", count);

	count = rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_BUF_PROTO]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_BUF_ICMP]);
	jsonw_uint_field(json, "etrans", count);

	count = 0;
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_S1_ENOMEM]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_S2_ENOMEM]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_PB_ENOMEM]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_SRC_ENOMEM]);
	jsonw_uint_field(json, "enomem", count);

	count = 0;
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_MBU_ENOSPC]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_SRC_ENOSPC]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_BLK_ENOSPC]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_APM_ENOSPC]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_POOL_ENOSPC]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_S1_ENOSPC]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_S2_ENOSPC]);
	jsonw_uint_field(json, "enospc", count);

	count = 0;
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_S1_EEXIST]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_S2_EEXIST]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_APM_ENOENT]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_SRC_ENOENT]);
	jsonw_uint_field(json, "ethread", count);

	count = 0;
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_BUF_ENOL3]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_BUF_ENOL4]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_BUF_ENOMEM]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_OUT][CGN_BUF_ENOSPC]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_IN][CGN_BUF_ENOL3]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_IN][CGN_BUF_ENOL4]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_IN][CGN_BUF_ENOMEM]);
	count += rte_atomic64_read(&cgn_errors[CGN_DIR_IN][CGN_BUF_ENOSPC]);
	jsonw_uint_field(json, "embuf", count);

	jsonw_end_object(json);
	jsonw_destroy(&json);
}

/*
 * Write json for errors in one direction
 */
static void cgn_show_errors_dir(json_writer_t *json, int dir, const char *name)
{
	uint64_t count;
	int err;

	jsonw_name(json, name);
	jsonw_start_array(json);

	for (err = 1; err <= CGN_ERRNO_LAST; err++) {
		jsonw_start_object(json);

		count = rte_atomic64_read(&cgn_errors[dir][err]);
		jsonw_string_field(json, "name", cgn_errno_str(err));
		jsonw_string_field(json, "desc", cgn_errno_detail_str(err));
		jsonw_uint_field(json, "errno", err);
		jsonw_uint_field(json, "count", count);

		jsonw_end_object(json);
	}

	jsonw_end_array(json);
}

/*
 * Write json for in and out errors
 */
static void cgn_show_errors(FILE *f, int argc __unused, char **argv __unused)
{
	json_writer_t *json;

	json = jsonw_new(f);
	if (!json)
		return;

	jsonw_name(json, "errors");
	jsonw_start_object(json);

	cgn_show_errors_dir(json, CGN_DIR_OUT, "out");
	cgn_show_errors_dir(json, CGN_DIR_IN, "in");

	jsonw_end_object(json);
	jsonw_destroy(&json);
}

/*
 * Unit-test specific op commands
 */
static int cgn_op_ut(FILE *f __unused, int argc, char **argv)
{
	if (argc < 3)
		return 0;

	if (!strcmp(argv[2], "gc"))
		cgn_session_gc_pass();

	return 0;
}

/*
 * cgn-op [ut] ....
 */
int cmd_cgn_op(FILE *f, int argc, char **argv)
{
	if (argc < 3)
		goto usage;

	/*
	 * Clear ...
	 */
	if (!strcmp(argv[1], "clear")) {
		if (!strcmp(argv[2], "session"))
			cgn_session_clear(f, argc, argv);

		return 0;
	}

	/*
	 * Show ...
	 */
	if (!strcmp(argv[1], "show")) {
		if (!strcmp(argv[2], "policy"))
			cgn_policy_show(f, argc, argv);

		else if (!strcmp(argv[2], "subscriber"))
			cgn_source_show(f, argc, argv);

		else if (!strcmp(argv[2], "session"))
			cgn_session_show(f, argc, argv);

		else if (!strcmp(argv[2], "apm"))
			apm_show(f, argc, argv);

		else if (!strcmp(argv[2], "errors"))
			cgn_show_errors(f, argc, argv);

		else if (!strcmp(argv[2], "summary"))
			cgn_show_summary(f, argc, argv);

		return 0;
	}

	/*
	 * List ...
	 */
	if (!strcmp(argv[1], "list")) {
		if (argc >= 4 &&
		    !strcmp(argv[2], "session") &&
		    !strcmp(argv[3], "id"))
			cgn_session_id_list(f, argc, argv);

		else if (!strcmp(argv[2], "subscribers"))
			cgn_source_list(f, argc, argv);

		else if (!strcmp(argv[2], "public"))
			apm_public_list(f, argc, argv);

		return 0;
	}

	/*
	 * Map ...
	 */
	if (!strcmp(argv[1], "map")) {
		cgn_op_session_map(f, argc, argv);
		return 0;
	}

	if (!strcmp(argv[1], "ut")) {
		cgn_op_ut(f, argc, argv);
		return 0;
	}

	return 0;

usage:
	if (f)
		fprintf(f, "%s: cgn-op {clear | show | list} ... ",
			__func__);

	return -1;
}
