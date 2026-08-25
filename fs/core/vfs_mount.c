#include "vfs_internal.h"
#include "kstring.h"

int vfs_mount(const char *source, const char *target, const char *fstype,
		unsigned long flags)
{
	(void)source; (void)flags;
	if (!target || !fstype || !vfs_is_dir(target)) return -1;
	if (kstrcmp(fstype, "proc") != 0 && kstrcmp(fstype, "sysfs") != 0 &&
		kstrcmp(fstype, "sys") != 0) return -1;
	for (int i = 0; i < VFS_MAX_MOUNTS; ++i) {
		if (mounts[i].used && kstrcmp(mounts[i].target, target) == 0) return -1;
	}
	for (int i = 0; i < VFS_MAX_MOUNTS; ++i) if (!mounts[i].used) {
		mounts[i].used = true;
		kstrcpy(mounts[i].target, target, sizeof(mounts[i].target));
		kstrcpy(mounts[i].fstype, fstype, sizeof(mounts[i].fstype));
		return 0;
	}
	return -1;
}

int vfs_umount(const char *target)
{
	if (!target) return -1;
	for (int i = 0; i < VFS_MAX_MOUNTS; ++i) if (mounts[i].used &&
		kstrcmp(mounts[i].target, target) == 0) { mounts[i].used = false; return 0; }
	return -1;
}

bool vfs_is_mounted(const char *target)
{
	if (!target) return false;
	for (int i = 0; i < VFS_MAX_MOUNTS; ++i)
		if (mounts[i].used && kstrcmp(mounts[i].target, target) == 0) return true;
	return false;
}

