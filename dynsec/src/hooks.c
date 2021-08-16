// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 VMware, Inc. All rights reserved.
#include <linux/binfmts.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include "dynsec.h"
#include "factory.h"
#include "stall_tbl.h"
#include "stall_reqs.h"
#include "lsm_mask.h"

int dynsec_bprm_set_creds(struct linux_binprm *bprm)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->bprm_set_creds(bprm);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_EXEC)) {
        goto out;
    }

    if (!bprm || !bprm->file) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_EXEC, DYNSEC_HOOK_TYPE_EXEC,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }
    if (!fill_in_bprm_set_creds(event, bprm, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

int dynsec_inode_unlink(struct inode *dir, struct dentry *dentry)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    umode_t mode;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_unlink(dir, dentry);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_UNLINK)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    // Only care about certain types of files
    if (!dentry->d_inode) {
        goto out;
    }
    mode = dentry->d_inode->i_mode;
    if (!(S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))) {
        goto out;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_UNLINK, DYNSEC_HOOK_TYPE_UNLINK,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }

    if (!fill_in_inode_unlink(event, dir, dentry, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

int dynsec_inode_rmdir(struct inode *dir, struct dentry *dentry)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    umode_t mode;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_rmdir(dir, dentry);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_RMDIR)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    // Only care about certain types of files
    if (!dentry->d_inode) {
        goto out;
    }
    mode = dentry->d_inode->i_mode;
    if (!(S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))) {
        goto out;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_RMDIR, DYNSEC_HOOK_TYPE_RMDIR,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }

    if (!fill_in_inode_unlink(event, dir, dentry, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

int dynsec_inode_rename(struct inode *old_dir, struct dentry *old_dentry,
                        struct inode *new_dir, struct dentry *new_dentry)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    umode_t mode;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_rename(old_dir, old_dentry,
                                               new_dir, new_dentry);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_RENAME)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    if (!old_dentry->d_inode) {
        goto out;
    }
    mode = old_dentry->d_inode->i_mode;
    if (!(S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))) {
        goto out;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_RENAME, DYNSEC_HOOK_TYPE_RENAME,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }

    if (!fill_in_inode_rename(event,
                              old_dir, old_dentry,
                              new_dir, new_dentry,
                              GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

int dynsec_inode_setattr(struct dentry *dentry, struct iattr *attr)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    unsigned int attr_mask;

    BUILD_BUG_ON(DYNSEC_SETATTR_MODE != ATTR_MODE);
    BUILD_BUG_ON(DYNSEC_SETATTR_UID  != ATTR_UID);
    BUILD_BUG_ON(DYNSEC_SETATTR_GID  != ATTR_GID);
    BUILD_BUG_ON(DYNSEC_SETATTR_SIZE != ATTR_SIZE);
    BUILD_BUG_ON(DYNSEC_SETATTR_FILE != ATTR_FILE);
    BUILD_BUG_ON(DYNSEC_SETATTR_OPEN != ATTR_OPEN);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_setattr(dentry, attr);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_SETATTR)) {
        goto out;
    }

    if (!dentry || !dentry->d_inode || !attr) {
        goto out;
    }

    attr_mask = attr->ia_valid;
    attr_mask &= (ATTR_MODE|ATTR_UID|ATTR_GID|ATTR_SIZE);

    if (!attr_mask) {
        goto out;
    }

    // Check for redundant fields
    if (attr_mask & ATTR_MODE) {
        // No need to check for subsets
        if (attr->ia_mode == dentry->d_inode->i_mode) {
            attr_mask &= ~(ATTR_MODE);
        }
    }
    if (attr_mask & ATTR_UID) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
        if (uid_eq(attr->ia_uid, dentry->d_inode->i_uid))
#else
        if (attr->ia_uid == dentry->d_inode->i_uid)
#endif
        {
            attr_mask &= ~(ATTR_UID);
        }
    }
    if (attr_mask & ATTR_GID) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
        if (gid_eq(attr->ia_gid, dentry->d_inode->i_gid))
#else
        if (attr->ia_gid == dentry->d_inode->i_gid)
#endif
        {
            attr_mask &= ~(ATTR_GID);
        }
    }
    if (attr_mask & ATTR_SIZE) {
        // Don't care about fallocate
        if (attr->ia_size) {
            attr_mask &= ~(ATTR_SIZE);
        }

        // Don't care if the file is already empty/truncated
        else if (attr->ia_size == dentry->d_inode->i_size) {
            attr_mask &= ~(ATTR_SIZE);
        }
    }

    if (!attr_mask) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_SETATTR, DYNSEC_HOOK_TYPE_SETATTR,
                               report_flags, GFP_KERNEL);

    if (!fill_in_inode_setattr(event, attr_mask,
                               dentry, attr, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int dynsec_inode_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
#else
int dynsec_inode_mkdir(struct inode *dir, struct dentry *dentry, int mode)
#endif
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_mkdir(dir, dentry, mode);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_MKDIR)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_MKDIR, DYNSEC_HOOK_TYPE_MKDIR,
                               report_flags, GFP_KERNEL);
    if (!fill_in_inode_mkdir(event, dir, dentry, mode, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int dynsec_inode_create(struct inode *dir, struct dentry *dentry,
                        umode_t mode)
#else
int dynsec_inode_create(struct inode *dir, struct dentry *dentry,
                        int mode)
#endif
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_create(dir, dentry, mode);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_CREATE)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_CREATE, DYNSEC_HOOK_TYPE_CREATE,
                               report_flags, GFP_KERNEL);
    if (!fill_in_inode_create(event, dir, dentry, mode, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

int dynsec_inode_link(struct dentry *old_dentry, struct inode *dir,
                      struct dentry *new_dentry)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_link(old_dentry, dir, new_dentry);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_LINK)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_LINK, DYNSEC_HOOK_TYPE_LINK,
                               report_flags, GFP_KERNEL);
    if (!fill_in_inode_link(event, old_dentry, dir, new_dentry, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

int dynsec_inode_symlink(struct inode *dir, struct dentry *dentry,
                const char *old_name)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret =  g_original_ops_ptr->inode_symlink(dir, dentry, old_name);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_SYMLINK)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_SYMLINK, DYNSEC_HOOK_TYPE_SYMLINK,
                               report_flags, GFP_KERNEL);
    if (!fill_in_inode_symlink(event, dir, dentry, old_name, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
int dynsec_file_open(struct file *file)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int dynsec_file_open(struct file *file, const struct cred *cred)
#else
int dynsec_dentry_open(struct file *file, const struct cred *cred)
#endif
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    if (g_original_ops_ptr) {
        ret =  g_original_ops_ptr->dentry_open(file, cred);
        if (ret) {
            goto out;
        }
    }
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->file_open(file, cred);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_OPEN)) {
        goto out;
    }

#ifdef FMODE_STREAM
    if (file->f_mode & FMODE_STREAM) {
        report_flags &= ~(DYNSEC_REPORT_STALL);
    }
#endif
#ifdef FMODE_NONOTIFY
    if ((file->f_mode & FMODE_NONOTIFY) && !(file->f_mode & FMODE_WRITE)) {
        report_flags &= ~(DYNSEC_REPORT_STALL);
    }
#endif

    // Some file systems and file types may not be
    // worth stalling or reporting against.
    // Below is a poorman's implementation.
    if (file->f_path.dentry && file->f_path.dentry->d_inode &&
        S_ISREG(file->f_path.dentry->d_inode->i_mode)) {
        report_flags = DYNSEC_REPORT_STALL;
    } else {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
        report_flags &= ~(DYNSEC_REPORT_STALL);
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_OPEN, DYNSEC_HOOK_TYPE_OPEN,
                               report_flags, GFP_KERNEL);
    if (!fill_in_file_open(event, file, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }

out:

    return ret;
}

// Must Not Stall - Enable only for open events
void dynsec_file_free_security(struct file *file)
{
    struct dynsec_event *event = NULL;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    if (g_original_ops_ptr) {
        g_original_ops_ptr->file_free_security(file);
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_CLOSE)) {
        return;
    }

#ifdef FMODE_STREAM
    if (file->f_mode & FMODE_STREAM) {
        return;
    }
#endif
#ifdef FMODE_NONOTIFY
    if ((file->f_mode & FMODE_NONOTIFY) && !(file->f_mode & FMODE_WRITE)) {
        return;
    }
#endif

    // Only report close events on
    if (!file->f_path.dentry || !file->f_path.dentry->d_inode ||
        !S_ISREG(file->f_path.dentry->d_inode->i_mode)) {
        return;
    }
    if (!stall_tbl_enabled(stall_tbl)) {
        return;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_CLOSE, DYNSEC_HOOK_TYPE_CLOSE,
                               report_flags, GFP_ATOMIC);

    if (!fill_in_file_free(event, file, GFP_ATOMIC)) {
        free_dynsec_event(event);
        return;
    }
    (void)enqueue_nonstall_event(stall_tbl, event);
}

int dynsec_ptrace_traceme(struct task_struct *parent)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->ptrace_traceme(parent);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_PTRACE)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_PTRACE, DYNSEC_HOOK_TYPE_PTRACE,
                               report_flags, GFP_ATOMIC);
    if (!fill_in_ptrace(event, parent, current)) {
        free_dynsec_event(event);
        goto out;
    }

    (void)enqueue_nonstall_event(stall_tbl, event);

out:

    return ret;
}

int dynsec_ptrace_access_check(struct task_struct *child, unsigned int mode)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->ptrace_access_check(child, mode);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_PTRACE)) {
        goto out;
    }

    if (!(mode & PTRACE_MODE_ATTACH)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else if (task_in_connected_tgid(child)) {
        // To prevent a feedback loop. Cache this context after first event.
        goto out;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_PTRACE, DYNSEC_HOOK_TYPE_PTRACE,
                               report_flags, GFP_ATOMIC);
    if (!fill_in_ptrace(event, current, child)) {
        free_dynsec_event(event);
        goto out;
    }

    (void)enqueue_nonstall_event(stall_tbl, event);

out:

    return ret;
}

// Must Not Stall
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
#if RHEL_MAJOR == 8 && RHEL_MINOR == 0
int dynsec_task_kill(struct task_struct *p, struct siginfo *info,
                     int sig, const struct cred *cred)
#else
int dynsec_task_kill(struct task_struct *p, struct kernel_siginfo *info,
                     int sig, const struct cred *cred)
#endif
#else
int dynsec_task_kill(struct task_struct *p, struct siginfo *info,
                     int sig, u32 secid)
#endif
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->task_kill(p, info, sig, secid);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!(lsm_hooks_enabled & DYNSEC_HOOK_TYPE_SIGNAL)) {
        goto out;
    }

    if (!sig) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_SIGNAL, DYNSEC_HOOK_TYPE_SIGNAL,
                               report_flags, GFP_ATOMIC);
    if (!fill_in_task_kill(event, p, sig)) {
        free_dynsec_event(event);
        goto out;
    }
    (void)enqueue_nonstall_event(stall_tbl, event);

out:

    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
void dynsec_sched_process_fork_tp(void *data, struct task_struct *parent,
                                  struct task_struct *child)
#else
void dynsec_sched_process_fork_tp(struct task_struct *parent,
                                  struct task_struct *child)
#endif
{
    struct dynsec_event *event = NULL;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

    if (!child) {
        return;
    }
    // Don't send thread events
    if (child->tgid != child->pid) {
        return;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        return;
    }
    if (task_in_connected_tgid(parent)) {
        report_flags |= DYNSEC_REPORT_SELF;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_CLONE, DYNSEC_TP_HOOK_TYPE_CLONE,
                               report_flags, GFP_ATOMIC);
    if (!fill_in_clone(event, parent, child)) {
        free_dynsec_event(event);
        return;
    }
    (void)enqueue_nonstall_event(stall_tbl, event);
}

static void __dynsec_task_exit(struct task_struct *task,
                               uint32_t exit_hook_type,
                               gfp_t mode)
{
    struct dynsec_event *event = NULL;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

    if (!task) {
        return;
    }
    // Don't send thread events
    if (task->tgid != task->pid) {
        return;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        return;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_EXIT, exit_hook_type,
                               report_flags, mode);
    if (!fill_task_free(event, task)) {
        free_dynsec_event(event);
        return;
    }

    // The common exit event should have to be high priority
    // as the task free event is always last.
    if (exit_hook_type == DYNSEC_TP_HOOK_TYPE_EXIT) {
        (void)enqueue_nonstall_event_low_pri(stall_tbl, event);
    } else {
        (void)enqueue_nonstall_event(stall_tbl, event);
    }
}
void dynsec_task_free(struct task_struct *task, uint32_t exit_hook_type)
{
    __dynsec_task_exit(task, DYNSEC_HOOK_TYPE_TASK_FREE, GFP_ATOMIC);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
void dynsec_sched_process_exit_tp(void *data, struct task_struct *task)
#else
void dynsec_sched_process_exit_tp(struct task_struct *task)
#endif
{
    __dynsec_task_exit(task, DYNSEC_TP_HOOK_TYPE_EXIT, GFP_ATOMIC);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
void dynsec_sched_process_free_tp(void *data, struct task_struct *task)
#else
void dynsec_sched_process_free_tp(struct task_struct *task)
#endif
{
    __dynsec_task_exit(task, DYNSEC_TP_HOOK_TYPE_TASK_FREE, GFP_ATOMIC);
}

// Settings to help control mmap event performance
int mmap_report_misc = 1;
int mmap_stall_misc = 0;
int mmap_stall_on_exec = 1;
int mmap_stall_on_ldso = 1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int dynsec_mmap_file(struct file *file, unsigned long reqprot, unsigned long prot,
                     unsigned long flags)
#else
int dynsec_file_mmap(struct file *file, unsigned long reqprot, unsigned long prot,
                     unsigned long flags, unsigned long addr, unsigned long addr_only)
#endif
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    bool is_low_priority = true;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->file_mmap(file, reqprot, prot, flags, addr, addr_only);
        if (ret) {
            goto out;
        }
    }
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->mmap_file(file, reqprot, prot, flags);
        if (ret) {
            goto out;
        }
    }
#endif

    if (!(prot & PROT_EXEC)) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }

    if (current->in_execve ||
        (file && (file->f_mode & FMODE_EXEC) == FMODE_EXEC)) {
        unsigned long exec_flags = flags & (MAP_DENYWRITE | MAP_EXECUTABLE);

        if (mmap_stall_on_exec && exec_flags & MAP_EXECUTABLE) {
            report_flags |= DYNSEC_REPORT_STALL;
        }
        else if (mmap_stall_on_ldso) {
            report_flags |= DYNSEC_REPORT_STALL;
        }
        is_low_priority = false;
    }
    else {
        if (mmap_stall_misc) {
            report_flags |= DYNSEC_REPORT_STALL;
        } else if (!mmap_report_misc) {
            goto out;
        }
    }

    // Don't stall on ourself
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
        report_flags &= ~(DYNSEC_REPORT_STALL);
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_MMAP, DYNSEC_HOOK_TYPE_MMAP,
                               report_flags, GFP_KERNEL);
    if (!fill_in_file_mmap(event, file, prot, flags, GFP_KERNEL)) {
        free_dynsec_event(event);
        goto out;
    }

    if (event->report_flags & DYNSEC_REPORT_STALL) {
        int response = 0;
        int rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

        if (!rc) {
            ret = response;
        }
    } else {
        if (is_low_priority) {
            (void)enqueue_nonstall_event_low_pri(stall_tbl, event);
        } else {
            (void)enqueue_nonstall_event(stall_tbl, event);
        }
    }

out:

    return ret;
}

struct kprobe;
int dynsec_wake_up_new_task(struct kprobe *kprobe, struct pt_regs *regs)
{
    struct dynsec_event *event = NULL;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
#ifdef CONFIG_HAVE_FUNCTION_ARG_ACCESS_API
    struct task_struct *p = regs_get_kernel_argument(regs, 0);
#else
    struct task_struct *p = (struct task_struct *)regs->di;
#endif

    if (!p) {
        goto out;
    }
    // Don't send thread events
    if (p->tgid != p->pid) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(p->real_parent)) {
        report_flags |= DYNSEC_REPORT_SELF;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_CLONE, DYNSEC_TP_HOOK_TYPE_CLONE,
                               report_flags, GFP_ATOMIC);
    if (!fill_in_clone(event, NULL, p)) {
        free_dynsec_event(event);
        goto out;
    }

    (void)enqueue_nonstall_event_low_pri(stall_tbl, event);

out:
    return 0;
}

// int dynsec_task_fix_setuid(struct cred *new, const struct cred *old, int flags)
// {
// #if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
//     if (g_original_ops_ptr) {
//         return g_original_ops_ptr->task_fix_setuid(new, old, flags);
//     }
// #endif

//     return 0;
// }