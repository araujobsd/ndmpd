/*
 * Copyright 2009 Sun Microsystems, Inc.  
 * Copyright 2015 Marcelo Araujo <araujo@FreeBSD.org>.
 * All rights reserved.
 *
 * Use is subject to license terms.
 */

/*
 * BSD 3 Clause License
 *
 * Copyright (c) 2007, The Storage Networking Industry Association.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in
 *        the documentation and/or other materials provided with the
 *        distribution.
 *
 *      - Neither the name of The Storage Networking Industry Association (SNIA)
 *        nor the names of its contributors may be used to endorse or promote
 *        products derived from this software without specific prior written
 *        permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MNTENT_H
#define	_MNTENT_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Mntfs io control commands
 */
#define	MNTIOC			('m' << 8)
#define	MNTIOC_NMNTS		(MNTIOC|1)	/* Get # of mounted resources */
#define	MNTIOC_GETDEVLIST	(MNTIOC|2)	/* Get mounted dev no.'s */
#define	MNTIOC_SETTAG		(MNTIOC|3)	/* Set a tag on a mounted fs */
#define	MNTIOC_CLRTAG		(MNTIOC|4)	/* Clear a tag from a fs */
#define	MNTIOC_SHOWHIDDEN	(MNTIOC|6)	/* private */
#define	MNTIOC_GETMNTENT	(MNTIOC|7)	/* private */
#define	MNTIOC_GETEXTMNTENT	(MNTIOC|8)	/* private */
#define	MNTIOC_GETMNTANY	(MNTIOC|9)	/* private */

#define	MNTTAB		"/etc/mnttab"
#define	VFSTAB		"/etc/vfstab"
#define	MNTMAXSTR	128

#define	MNTTYPE_ZFS	"zfs"		/* ZFS file system */
#define	MNTTYPE_UFS	"ufs"		/* Unix file system */
#define	MNTTYPE_SMBFS	"smbfs"		/* SMBFS file system */
#define	MNTTYPE_NFS	"nfs"		/* NFS file system */
#define	MNTTYPE_NFS3	"nfs3"		/* NFS Version 3 file system */
#define	MNTTYPE_NFS4	"nfs4"		/* NFS Version 4 file system */
#define	MNTTYPE_CACHEFS	"cachefs"	/* Cache File System */
#define	MNTTYPE_PCFS	"pcfs"		/* PC (MSDOS) file system */
#define	MNTTYPE_PC	MNTTYPE_PCFS	/* Deprecated name; use MNTTYPE_PCFS */
#define	MNTTYPE_LOFS	"lofs"		/* Loop back file system */
#define	MNTTYPE_LO	MNTTYPE_LOFS	/* Deprecated name; use MNTTYPE_LOFS */
#define	MNTTYPE_HSFS	"hsfs"		/* High Sierra (9660) file system */
#define	MNTTYPE_SWAP	"swap"		/* Swap file system */
#define	MNTTYPE_TMPFS	"tmpfs"		/* Tmp volatile file system */
#define	MNTTYPE_AUTOFS	"autofs"	/* Automounter ``file'' system */
#define	MNTTYPE_MNTFS	"mntfs"		/* In-kernel mnttab */
#define	MNTTYPE_DEV	"dev"		/* /dev file system */
#define	MNTTYPE_CTFS	"ctfs"		/* Contract file system */
#define	MNTTYPE_OBJFS	"objfs"		/* Kernel object file system */
#define	MNTTYPE_SHAREFS	"sharefs"	/* Kernel sharetab file system */


#define	MNTOPT_RO	"ro"		/* Read only */
#define	MNTOPT_RW	"rw"		/* Read/write */
#define	MNTOPT_RQ	"rq"		/* Read/write with quotas */
#define	MNTOPT_QUOTA	"quota"		/* Check quotas */
#define	MNTOPT_NOQUOTA	"noquota"	/* Don't check quotas */
#define	MNTOPT_ONERROR	"onerror"	/* action to taken on error */
#define	MNTOPT_SOFT	"soft"		/* Soft mount */
#define	MNTOPT_SEMISOFT	"semisoft"	/* partial soft, uncommited interface */
#define	MNTOPT_HARD	"hard"		/* Hard mount */
#define	MNTOPT_SUID	"suid"		/* Both setuid and devices allowed */
#define	MNTOPT_NOSUID	"nosuid"	/* Neither setuid nor devices allowed */
#define	MNTOPT_DEVICES	"devices"	/* Device-special allowed */
#define	MNTOPT_NODEVICES	"nodevices"	/* Device-special disallowed */
#define	MNTOPT_SETUID	"setuid"	/* Set uid allowed */
#define	MNTOPT_NOSETUID	"nosetuid"	/* Set uid not allowed */
#define	MNTOPT_GRPID	"grpid"		/* SysV-compatible gid on create */
#define	MNTOPT_REMOUNT	"remount"	/* Change mount options */
#define	MNTOPT_NOSUB	"nosub"		/* Disallow mounts on subdirs */
#define	MNTOPT_MULTI	"multi"		/* Do multi-component lookup */
#define	MNTOPT_INTR	"intr"		/* Allow NFS ops to be interrupted */
#define	MNTOPT_NOINTR	"nointr"	/* Don't allow interrupted ops */
#define	MNTOPT_PORT	"port"		/* NFS server IP port number */
#define	MNTOPT_SECURE	"secure"	/* Secure (AUTH_DES) mounting */
#define	MNTOPT_RSIZE	"rsize"		/* Max NFS read size (bytes) */
#define	MNTOPT_WSIZE	"wsize"		/* Max NFS write size (bytes) */
#define	MNTOPT_TIMEO	"timeo"		/* NFS timeout (1/10 sec) */
#define	MNTOPT_RETRANS	"retrans"	/* Max retransmissions (soft mnts) */
#define	MNTOPT_ACTIMEO	"actimeo"	/* Attr cache timeout (sec) */
#define	MNTOPT_ACREGMIN	"acregmin"	/* Min attr cache timeout (files) */
#define	MNTOPT_ACREGMAX	"acregmax"	/* Max attr cache timeout (files) */
#define	MNTOPT_ACDIRMIN	"acdirmin"	/* Min attr cache timeout (dirs) */
#define	MNTOPT_ACDIRMAX	"acdirmax"	/* Max attr cache timeout (dirs) */
#define	MNTOPT_NOAC	"noac"		/* Don't cache attributes at all */
#define	MNTOPT_NOCTO	"nocto"		/* No close-to-open consistency */
#define	MNTOPT_BG	"bg"		/* Do mount retries in background */
#define	MNTOPT_FG	"fg"		/* Do mount retries in foreground */
#define	MNTOPT_RETRY	"retry"		/* Number of mount retries */
#define	MNTOPT_DEV	"dev"		/* Device id of mounted fs */
#define	MNTOPT_POSIX	"posix"		/* Get static pathconf for mount */
#define	MNTOPT_MAP	"map"		/* Automount map */
#define	MNTOPT_DIRECT	"direct"	/* Automount   direct map mount */
#define	MNTOPT_INDIRECT	"indirect"	/* Automount indirect map mount */
#define	MNTOPT_LLOCK	"llock"		/* Local locking (no lock manager) */
#define	MNTOPT_IGNORE	"ignore"	/* Ignore this entry */
#define	MNTOPT_VERS	"vers"		/* protocol version number indicator */
#define	MNTOPT_PROTO	"proto"		/* protocol network_id indicator */
#define	MNTOPT_SEC	"sec"		/* Security flavor indicator */
#define	MNTOPT_SYNCDIR	"syncdir"	/* Synchronous local directory ops */
#define	MNTOPT_NOSETSEC	"nosec"		/* Do no allow setting sec attrs */
#define	MNTOPT_NOPRINT	"noprint"	/* Do not print messages */
#define	MNTOPT_LARGEFILES "largefiles"	/* allow large files */
#define	MNTOPT_NOLARGEFILES "nolargefiles" /* don't allow large files */
#define	MNTOPT_FORCEDIRECTIO "forcedirectio" /* Force DirectIO on all files */
#define	MNTOPT_NOFORCEDIRECTIO "noforcedirectio" /* No Force DirectIO */
#define	MNTOPT_DISABLEDIRECTIO "disabledirectio" /* Disable DirectIO ioctls */
#define	MNTOPT_PUBLIC	"public"	/* Use NFS public file handlee */
#define	MNTOPT_LOGGING "logging" 	/* enable logging */
#define	MNTOPT_NOLOGGING "nologging" 	/* disable logging */
#define	MNTOPT_ATIME	"atime"		/* update atime for files */
#define	MNTOPT_NOATIME  "noatime"	/* do not update atime for files */
#define	MNTOPT_GLOBAL	"global"	/* Cluster-wide global mount */
#define	MNTOPT_NOGLOBAL	"noglobal"	/* Mount local to single node */
#define	MNTOPT_DFRATIME	"dfratime"	/* Deferred access time updates */
#define	MNTOPT_NODFRATIME "nodfratime"	/* No Deferred access time updates */
#define	MNTOPT_NBMAND	"nbmand"	/* allow non-blocking mandatory locks */
#define	MNTOPT_NONBMAND	"nonbmand"	/* deny non-blocking mandatory locks */
#define	MNTOPT_XATTR	"xattr"		/* enable extended attributes */
#define	MNTOPT_NOXATTR	"noxattr"	/* disable extended attributes */
#define	MNTOPT_EXEC	"exec"		/* enable executables */
#define	MNTOPT_NOEXEC	"noexec"	/* disable executables */
#define	MNTOPT_RESTRICT	"restrict"	/* restricted autofs mount */
#define	MNTOPT_BROWSE	"browse"	/* browsable autofs mount */
#define	MNTOPT_NOBROWSE	"nobrowse"	/* non-browsable autofs mount */
#define	MNTOPT_ZONE	"zone"	/* zone name - set only for non global zones */

struct mnttab {
	char	*mnt_special;
	char	*mnt_mountp;
	char	*mnt_fstype;
	char	*mnt_mntopts;
};

#ifdef	__cplusplus
}
#endif

#endif	/* _MNTENT_H */
