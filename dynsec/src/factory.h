/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
// Copyright (c) 2021 VMware, Inc. All rights reserved.

#pragma once

#include "dynsec.h"

extern uint32_t debug_disable_stall_mask;

#pragma pack(push, 1)
// Helper kernel structs in queue
struct dynsec_exec_kmsg {
    struct dynsec_msg_hdr hdr;
    struct dynsec_exec_msg msg;
    char *path;
};

struct dynsec_unlink_kmsg {
    struct dynsec_msg_hdr hdr;
    struct dynsec_unlink_msg msg;
    char *path;
};

struct dynsec_rename_kmsg {
    struct dynsec_msg_hdr hdr;
    struct dynsec_rename_msg msg;
    char *old_path;
    char *new_path;
};


// Base struct in queue
struct dynsec_event {
    uint32_t tid;
    uint64_t req_id;
    enum dynsec_event_type event_type;
    struct list_head list;
};

// Child event structs
struct dynsec_exec_event {
    struct dynsec_event event;
    struct dynsec_exec_kmsg kmsg;
};

struct dynsec_unlink_event {
    struct dynsec_event event;
    struct dynsec_unlink_kmsg kmsg;
};

struct dynsec_rename_event {
    struct dynsec_event event;
    struct dynsec_rename_kmsg kmsg;
};
#pragma pack(pop)

// Exec Event container_of helper
static inline struct dynsec_exec_event *
dynsec_event_to_exec(const struct dynsec_event *dynsec_event)
{
    return container_of(dynsec_event, struct dynsec_exec_event, event);
}

static inline struct dynsec_unlink_event *
dynsec_event_to_unlink(const struct dynsec_event *dynsec_event)
{
    return container_of(dynsec_event, struct dynsec_unlink_event, event);
}

static inline struct dynsec_rename_event *
dynsec_event_to_rename(const struct dynsec_event *dynsec_event)
{
    return container_of(dynsec_event, struct dynsec_rename_event, event);
}

extern uint16_t get_dynsec_event_payload(struct dynsec_event *dynsec_event);

extern struct dynsec_event *alloc_dynsec_event(enum dynsec_event_type event_type,
                                               uint32_t hook_type,
                                               uint16_t report_flags,
                                               gfp_t mode);

extern void free_dynsec_event(struct dynsec_event *dynsec_event);

extern ssize_t copy_dynsec_event_to_user(const struct dynsec_event *dynsec_event,
                                         char *__user p, size_t count);

// Event fillers
#include <linux/binfmts.h>
extern bool fill_in_bprm_set_creds(struct dynsec_exec_event *exec_event,
                                   const struct linux_binprm *bprm, gfp_t mode);

extern bool fill_in_inode_unlink(struct dynsec_unlink_event *unlink_event,
                                 struct inode *dir, struct dentry *dentry, gfp_t mode);

extern bool fill_in_inode_rename(struct dynsec_rename_event *rename_event,
                                 struct inode *old_dir, struct dentry *old_dentry,
                                 struct inode *new_dir, struct dentry *new_dentry,
                                 gfp_t mode);