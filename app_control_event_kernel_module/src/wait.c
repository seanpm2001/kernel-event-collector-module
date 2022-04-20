// SPDX-License-Identifier: GPL-2.0
// Copyright 2022 VMware, Inc. All rights reserved.

#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/wait.h>

#include "stall_tbl.h"
#include "stall_reqs.h"
#include "config.h"
#include "inode_cache.h"
#include "task_cache.h"

#define MAX_CONTINUE_RESPONSES 256

int dynsec_debug_stall = 0;

static int do_stall_interruptible(struct stall_entry *entry, int *response)
{
    int ret = 0;
    int wait_ret;
    int local_response;
    unsigned long local_timeout;
    unsigned long timeout;
    unsigned int continue_count = 0;
    int default_reponse = entry->response;

    // Initial values before we might perform a continuation
    timeout = msecs_to_jiffies(get_wait_timeout());
    local_response = entry->response;

retry:
    if (!stall_tbl_enabled(stall_tbl)) {
        return -ECHILD;
    }
    if (!stall_mode_enabled()) {
        return -ECHILD;
    }
    if (bypass_mode_enabled()) {
        return -ECHILD;
    }
    local_timeout = 0;
    local_response = default_reponse;

    // entry->mode could be an atomic
    wait_ret = wait_event_interruptible_timeout(entry->wq,
                                                (entry->mode != DYNSEC_STALL_MODE_STALL),
                                                timeout);
    // Interrupt
    if (wait_ret < 0) {
        // We could opt for a non-deny response here or
        // set back to safe value.

        if (dynsec_debug_stall) {
            pr_info("%s: interruped %d\n", __func__, wait_ret);
        }
    }
    // Timedout and conditional not met in time
    else if (wait_ret == 0) {
        // Where default response is desired most and hit most frequently

        if (dynsec_debug_stall) {
            pr_info("%s:%d response:%d timedout:%lu jiffies\n", __func__, __LINE__,
                    local_response, timeout);
        }
    }
    // Conditional was true, likely wake_up
    else {
        // Acts more like a memory barrier.
        // Copy all data needed for possible continuation.
        spin_lock(&entry->lock);
        local_response = entry->response;
        local_timeout = entry->stall_timeout;

        // reset mode back to stall will definitely require spin_lock
        entry->mode = DYNSEC_STALL_MODE_STALL;
        // Could copy over requested custom continuation timeout
        spin_unlock(&entry->lock);

        // Userspace wants to extend stalling of this task
        if (local_response == DYNSEC_RESPONSE_CONTINUE) {
            if (local_timeout) {
                timeout = msecs_to_jiffies(local_timeout);
            } else {
                timeout = msecs_to_jiffies(get_continue_timeout());
            }
            continue_count += 1;
            if (dynsec_debug_stall) {
                pr_info("%s:%d continue:%u extending stall:%lu jiffies\n",
                        __func__, __LINE__, continue_count, timeout);
            }

            // Don't let userspace ping/pong for too long
            if (continue_count < MAX_CONTINUE_RESPONSES) {
                goto retry;
            }
            ret = -ECHILD;
        }
    }

    if (local_response == DYNSEC_RESPONSE_EPERM) {
        *response = -EPERM;
    }

    // Must always attempt to remove from the table unless some entry
    // state in the future tells we don't have to.
    stall_tbl_remove_entry(stall_tbl, entry);

    return ret;
}

int dynsec_wait_event_timeout(struct dynsec_event *dynsec_event, int *response,
                              gfp_t mode)
{
    struct stall_entry *entry;

    if (!response) {
        return -EINVAL;
    }

    // Regardless default timeout return value,
    // set return value to a safe value.
    *response = 0;

    if (!dynsec_event || !stall_tbl_enabled(stall_tbl)) {
        free_dynsec_event(dynsec_event);
        return -EINVAL;
    }

    // Not the cleanest place to check
    if ((dynsec_event->report_flags & DYNSEC_REPORT_IGNORE) &&
        ignore_mode_enabled()) {
        free_dynsec_event(dynsec_event);
        return -ECHILD;
    }

    entry = stall_tbl_insert(stall_tbl, dynsec_event, mode);
    if (IS_ERR(entry)) {
        free_dynsec_event(dynsec_event);
        return PTR_ERR(entry);
    }

    if (entry) {
        (void)do_stall_interruptible(entry, response);
        kfree(entry);
    }

    return 0;
}


int handle_stall_ioc(const struct dynsec_stall_ioc_hdr *hdr)
{
    unsigned long flags = 0;

    if (!hdr) {
        return -EINVAL;
    }

    flags = hdr->flags;
    flags &= (DYNSEC_STALL_MODE_SET
        | DYNSEC_STALL_DEFAULT_TIMEOUT
        | DYNSEC_STALL_CONTINUE_TIMEOUT
        | DYNSEC_STALL_DEFAULT_DENY
    );
    if (!flags) {
        return -EINVAL;
    }

    if (!capable(CAP_SYS_ADMIN)) {
        return -EPERM;
    }

    lock_config();
    if (flags & DYNSEC_STALL_MODE_SET) {
        if (stall_mode_enabled()) {
            // Disable stalling
            if (hdr->stall_mode == DEFAULT_DISABLED) {
                global_config.stall_mode = DEFAULT_DISABLED;
                task_cache_clear();
                inode_cache_clear();
            }
        } else {
            // Enable stalling
            if (hdr->stall_mode != DEFAULT_DISABLED) {
                task_cache_clear();
                inode_cache_clear();
                global_config.stall_mode = DEFAULT_ENABLED;
            }
        }
    }
    if (flags & DYNSEC_STALL_DEFAULT_TIMEOUT) {
        unsigned long timeout_ms = MAX_WAIT_TIMEOUT_MS;

        if (hdr->stall_timeout < MAX_WAIT_TIMEOUT_MS) {
            timeout_ms = hdr->stall_timeout;
        }
        if (timeout_ms < MIN_WAIT_TIMEOUT_MS) {
            timeout_ms = MIN_WAIT_TIMEOUT_MS;
        }

        global_config.stall_timeout = timeout_ms;
    }
    if (flags & DYNSEC_STALL_CONTINUE_TIMEOUT) {
        unsigned long timeout_ms = MAX_WAIT_TIMEOUT_MS;

        // Ensure our continuation timeout as at least as long as
        // the regular timeout.
        if (hdr->stall_timeout_continue > global_config.stall_timeout) {
            timeout_ms = hdr->stall_timeout_continue;
        } else {
            timeout_ms = global_config.stall_timeout;
        }
        if (timeout_ms > MAX_EXTENDED_TIMEOUT_MS) {
            timeout_ms = MAX_EXTENDED_TIMEOUT_MS;
        }

        global_config.stall_timeout_continue = timeout_ms;
    }

    if (flags & DYNSEC_STALL_DEFAULT_DENY) {
        if (deny_on_timeout_enabled()) {
            // Turn off Default Deny
            if (hdr->stall_timeout_deny == DEFAULT_DISABLED) {
                global_config.stall_timeout_deny = DEFAULT_DISABLED;
            }
        } else {
            // Turn on Default Deny
            if (hdr->stall_timeout_deny != DEFAULT_DISABLED) {
                global_config.stall_timeout_deny = DEFAULT_ENABLED;
            }
        }
    }
    unlock_config();

    return 0;
}