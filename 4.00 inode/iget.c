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

/*
 * Find an inode if it is incore.
 * This is the equivalent, for inodes,
 * of ``incore'' in bio.c or ``pfind'' in subr.c.
 */
struct inode *ifind(dev_t dev, ino_t ino, int fstyp)
{
        struct inode *ip;
        
        for (ip = &inode[inohash[INOHASH(dev, ino, fstyp)]]; ip != &inode[-1]; ip = &inode[ip->i_hlink])
                if (ino == ip->i_number && dev == ip->i_dev)
                        return (ip);
        return ((struct inode *)0);
}

