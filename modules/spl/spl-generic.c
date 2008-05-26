/*
 *  This file is part of the SPL: Solaris Porting Layer.
 *
 *  Copyright (c) 2008 Lawrence Livermore National Security, LLC.
 *  Produced at Lawrence Livermore National Laboratory
 *  Written by:
 *          Brian Behlendorf <behlendorf1@llnl.gov>,
 *          Herb Wartens <wartens2@llnl.gov>,
 *          Jim Garlick <garlick@llnl.gov>
 *  UCRL-CODE-235197
 *
 *  This is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <sys/sysmacros.h>
#include <sys/vmsystm.h>
#include <sys/vnode.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/debug.h>
#include <sys/proc.h>
#include <sys/kstat.h>
#include <linux/kmod.h>
#include "config.h"

#ifdef DEBUG_SUBSYSTEM
#undef DEBUG_SUBSYSTEM
#endif

#define DEBUG_SUBSYSTEM S_GENERIC

char spl_version[16] = "SPL v" VERSION;

long spl_hostid = 0;
EXPORT_SYMBOL(spl_hostid);

char hw_serial[11] = "<none>";
EXPORT_SYMBOL(hw_serial);

int p0 = 0;
EXPORT_SYMBOL(p0);

vmem_t *zio_alloc_arena = NULL;
EXPORT_SYMBOL(zio_alloc_arena);

int
highbit(unsigned long i)
{
        register int h = 1;
	ENTRY;

        if (i == 0)
                RETURN(0);
#if BITS_PER_LONG == 64
        if (i & 0xffffffff00000000ul) {
                h += 32; i >>= 32;
        }
#endif
        if (i & 0xffff0000) {
                h += 16; i >>= 16;
        }
        if (i & 0xff00) {
                h += 8; i >>= 8;
        }
        if (i & 0xf0) {
                h += 4; i >>= 4;
        }
        if (i & 0xc) {
                h += 2; i >>= 2;
        }
        if (i & 0x2) {
                h += 1;
        }
        RETURN(h);
}
EXPORT_SYMBOL(highbit);

int
ddi_strtoul(const char *str, char **nptr, int base, unsigned long *result)
{
        char *end;
        return (*result = simple_strtoul(str, &end, base));
}
EXPORT_SYMBOL(ddi_strtoul);

static int
set_hostid(void)
{
	char sh_path[] = "/bin/sh";
	char *argv[] = { sh_path,
	                 "-c",
	                 "/usr/bin/hostid >/proc/sys/spl/hostid",
	                 NULL };
	char *envp[] = { "HOME=/",
	                 "TERM=linux",
	                 "PATH=/sbin:/usr/sbin:/bin:/usr/bin",
	                 NULL };

	/* Doing address resolution in the kernel is tricky and just
	 * not a good idea in general.  So to set the proper 'hw_serial'
	 * use the usermodehelper support to ask '/bin/sh' to run
	 * '/usr/bin/hostid' and redirect the result to /proc/sys/spl/hostid
	 * for us to use.  It's a horific solution but it will do for now.
	 */
	return call_usermodehelper(sh_path, argv, envp, 1);
}

static int __init spl_init(void)
{
	int rc = 0;

	if ((rc = debug_init()))
		return rc;

	if ((rc = kmem_init()))
		GOTO(out , rc);

	if ((rc = spl_mutex_init()))
		GOTO(out2 , rc);

	if ((rc = vn_init()))
		GOTO(out3, rc);

	if ((rc = proc_init()))
		GOTO(out4, rc);

	if ((rc = kstat_init()))
		GOTO(out5, rc);

	if ((rc = set_hostid()))
		GOTO(out6, rc = -EADDRNOTAVAIL);

	printk("SPL: Loaded Solaris Porting Layer v%s\n", VERSION);
	RETURN(rc);
out6:
	kstat_fini();
out5:
	proc_fini();
out4:
	vn_fini();
out3:
	spl_mutex_fini();
out2:
	kmem_fini();
out:
	debug_fini();

	printk("SPL: Failed to Load Solaris Porting Layer v%s, "
	       "rc = %d\n", VERSION, rc);
	return rc;
}

static void spl_fini(void)
{
	ENTRY;

	printk("SPL: Unloaded Solaris Porting Layer v%s\n", VERSION);
	kstat_fini();
	proc_fini();
	vn_fini();
	kmem_fini();
	debug_fini();
}

module_init(spl_init);
module_exit(spl_fini);

MODULE_AUTHOR("Lawrence Livermore National Labs");
MODULE_DESCRIPTION("Solaris Porting Layer");
MODULE_LICENSE("GPL");
