// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* a QEMU TCG plugin for scripts/benchmark: the guest instructions run, as QEMU's tests/plugin/insn.c counts them (each translated
   block adds its count inline when it runs), printed as "insns N" at exit w/ -d plugin; a line per instruction (-d exec) took
   hours. Built for the host; QEMU 8's plugin API (qemu-plugin.h, version 1) declared here */
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint64_t qemu_plugin_id_t;
struct qemu_info_t;
struct qemu_plugin_tb;
enum qemu_plugin_op { QEMU_PLUGIN_INLINE_ADD_U64 };
typedef void (*qemu_plugin_vcpu_tb_trans_cb_t)(qemu_plugin_id_t id, struct qemu_plugin_tb *tb);
typedef void (*qemu_plugin_udata_cb_t)(qemu_plugin_id_t id, void *userdata);
void qemu_plugin_register_vcpu_tb_trans_cb(qemu_plugin_id_t id, qemu_plugin_vcpu_tb_trans_cb_t cb);
void qemu_plugin_register_vcpu_tb_exec_inline(struct qemu_plugin_tb *tb, enum qemu_plugin_op op, void *ptr, uint64_t imm);
size_t qemu_plugin_tb_n_insns(const struct qemu_plugin_tb *tb);
void qemu_plugin_register_atexit_cb(qemu_plugin_id_t id, qemu_plugin_udata_cb_t cb, void *userdata);
void qemu_plugin_outs(const char *string);

__attribute__((visibility("default"))) int qemu_plugin_version = 1;

static uint64_t insns; /* one thread: the benchees are single-threaded */

static void translated(qemu_plugin_id_t id, struct qemu_plugin_tb *tb){
	(void)id;
	qemu_plugin_register_vcpu_tb_exec_inline(tb, QEMU_PLUGIN_INLINE_ADD_U64, &insns, qemu_plugin_tb_n_insns(tb));
}

static void finished(qemu_plugin_id_t id, void *userdata){
	(void)id;
	(void)userdata;
	char line[48];
	snprintf(line, sizeof(line), "insns %" PRIu64 "\n", insns);
	qemu_plugin_outs(line);
}

__attribute__((visibility("default"))) int qemu_plugin_install(qemu_plugin_id_t id, const struct qemu_info_t *info, int argc,
															   char **argv){
	(void)info;
	(void)argc;
	(void)argv;
	qemu_plugin_register_vcpu_tb_trans_cb(id, translated);
	qemu_plugin_register_atexit_cb(id, finished, NULL);
	return 0;
}
