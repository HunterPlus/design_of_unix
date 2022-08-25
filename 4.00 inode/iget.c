#include "inode.h"

/* kernel */
struct inode *inode, *inodeNINODE;
int     ninode;
struct inode *rootdir;          /* pointer to inode of root directory */

#define INOHSZ  63
#define INOHASH(dev, ino, fstype) (((dev)+(ino)+(fstype)) % INOHSZ)
short   inohash[INOHSZ];
short   ifreel;

/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
void ihinit()
{
        int     i;
        struct inode *ip = inode;

        ifreel = 0;
        for (i = 0; i < ninode-1; i++, ip++)
                ip->i_hlink = i+1;
        ip->i_hlink = -1;
        for (i = 0; i < INOHSZ; i++)
                inohash[i] = -1;
}