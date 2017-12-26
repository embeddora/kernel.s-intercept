/*
 * Copyright (c) 2018 [n/a] info@embeddora.com All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in the
 *          documentation and/or other materials provided with the distribution.
 *        * Neither the name of The Linux Foundation nor
 *          the names of its contributors may be used to endorse or promote
 *          products derived from this software without specific prior written
 *          permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Macro PATH_MAX */
#include <linux/syscalls.h>

/* Macros <MODULE_LICENSE>, <MODULE_AUTHOR>, <MODULE_DESCRIPTION> */
#include <linux/module.h>

/* Address of System-call table */
unsigned long *uloSct; 

/* General Protection Faults 'off' on kernels up to 3.x */
#define DisableGPF write_cr0(read_cr0() & (~0x10000))

/* General Protection Faults 'on' on kernels up to 3.x */
#define EnableGPF write_cr0(read_cr0() | 0x10000)

/* Module name. TODO: make it shorter */
static char rgModname[] = "Syscall-5 hooker for MPCC";

/* Buffer for forging a target directory */
static char rgPlaceholder[PATH_MAX];

/* Function pointer to original <__NR_open> syscall */
asmlinkage int (*al_iOrigopen)(char * fname, int iFlags, mode_t iMode);

/* Magic sequence to replace with our sequence (not less magic than initial one) */
#define MAGIC_SEQNS	"/proc/"

/* On installation this fn. be called everytime anybody aims to access a file from '/proc' */
asmlinkage int iHookedOpen(char * cpFname, int iFlags, mode_t iMode)
{
	/* Somebody tries to 'open' files from 'magic' dir? Then:*/
	if (NULL != strstr(cpFname, MAGIC_SEQNS ) )
	{
		/* a) clean the directory string buffer; */
		memset(rgPlaceholder, 0, PATH_MAX);

		/* b) replace original 'magic' dir with our own one; */
		strcpy(& ( rgPlaceholder[0]), "/pr@c/"); strcat(& ( rgPlaceholder[6]), & ( cpFname[6]) );

		printk(KERN_ALERT "[%s] orig <%s>  forged <%s>\n", rgModname, cpFname, rgPlaceholder);

		/* c) 'open' a desired file in our own directory (assuming <MPCC> has placed it over there) */
		return al_iOrigopen(& ( rgPlaceholder[0]), iFlags, iMode);	    
	}
	else
		/* In rest cases let the original 'open' finction do the job*/
		return al_iOrigopen(cpFname, iFlags, iMode);

} /* asmlinkage int iHookedOpen(char * cpFname, int iFlags, mode_t iMode) */

//----------------

//char data[2877917];
#if (0)
unsigned long uloGetSysCallTableAddr(const char * cpFname)
{
struct file *filp = NULL;
mm_segment_t oldfs;
int err = 0;

return 0;
	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(cpFname, O_RDONLY, 0);
	set_fs(oldfs);
# if 0
	if (IS_ERR(filp))
	{
		err = PTR_ERR(filp);
		return 0;
	}
	else
	{
	unsigned long long offset;
	unsigned int size = 2877917;

		mm_segment_t oldfs;
		int ret;

		oldfs = get_fs();
		set_fs(get_ds());

		ret = vfs_read(filp, data, size, &offset);

		set_fs(oldfs);
//---		return ret;
		if (0 != ret)
		{
		char * pfoud;
			if (NULL != (pfoud = strstr(data, "sys_call_table" ) ) )
			{
				int iPos = (int) (pfoud - data) ;
				printk(KERN_ALERT "[%s] Addr = #%d.\n", rgModname, iPos);
			}
			else
				printk(KERN_ALERT "[%s] Nothing was found \n", rgModname);
		}
		else
			return 0;
	}   
#endif /* (0) */

//---    return filp;

    filp_close(filp, NULL);

}
#endif /* (0) */

//----------------



/* Function to be executed on <insmod, modprobe> */
static int __init sc5hooker_init(void)
{
	/* Self-explanatory */
	printk(KERN_ALERT "[%s] Replacing system call #%d with function customized for MPCC-kit\n", rgModname, __NR_open);

	/* Disable kernel protection of pages */
	DisableGPF;

#if (0)
	uloGetSysCallTableAddr("/boot/System.map-3.19.0-25-generic");
#else
	/* Assign what was extracted with 'sudo grep sys_call_table /boot/System.map-3.19.0-25-generic' command to <uloSct> variable */
	uloSct = (unsigned long*) 0xc16cc140; 
#endif /* (0) */

	/* Preserve original function pointer in <al_iOrigopen> variable */
	al_iOrigopen = (void*) uloSct[__NR_open]; 

	/* Self-explanatory */
	printk(KERN_ALERT "[%s] System call #%d preserved\n", rgModname, __NR_open);

	/* Rewrite original function pointer with pointer onto <iHookedOpen> function */
	uloSct[__NR_open] = (unsigned long) iHookedOpen;

	/* Self-explanatory */
	printk(KERN_ALERT "[%s] System call #%d reassigned\n", rgModname, __NR_open);

	/* Allow kernel protection of pages */
	EnableGPF;

	return 0;

} /* static int __init sc5hooker_init(void) */

/* Function to be executed on <rmmod> */
static void __exit sc5hooker_cleanup(void)
{
	/* Self-explanatory */
	printk(KERN_ALERT "[%s] Restoring original system call #%d.\n", rgModname, __NR_open);

	/* Disable kernel protection of pages */
	DisableGPF;

	if (NULL != uloSct)
	{
		/* Assign original function pointer */
		uloSct[__NR_open] = (unsigned long) al_iOrigopen;

		/* Make user pacified */
		printk(KERN_ALERT "[%s] System call #%d restored to its original state. Leaving. \n", rgModname, __NR_open);
	}
	else
		/* Leaving 'as is' - nothing to do anyway */
		printk(KERN_ALERT "[%s] On module cleanup a System-call table was incorrect. Actually it's a rare case, (but as we've get nothing to do with it) leaving without any action.\n", rgModname);

	/* Allow kernel protection of pages */
	EnableGPF;

} /* static void __exit sc5hooker_cleanup(void) */


/* To not 'taint' the kernel let's define 'GNU Public License v2 and more' */
MODULE_LICENSE("GPL and additional rights");

/* Author's badge data */
MODULE_AUTHOR("Software Developer, <info@embeddora.com>, [n/a]");

/* For those inquisitive running 'modinfo' to learn what the module is purposed for */
MODULE_DESCRIPTION("Module to substitute <open(\"/proc/XXXXXX\")> with <open(\"/pr@c/XXXXXX\")>");


/* Module insertion into a kernel */
module_init(sc5hooker_init);

/* Module removal from a kernel */
module_exit(sc5hooker_cleanup);
