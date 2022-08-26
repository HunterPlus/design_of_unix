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
 * This is the equivalent, for inodes,  of ``incore'' in bio.c or ``pfind'' in subr.c.
 */
struct inode *ifind(dev_t dev, ino_t ino, int fstyp)
{
        struct inode *ip;
        
        for (ip = &inode[inohash[INOHASH(dev, ino, fstyp)]]; ip != &inode[-1]; ip = &inode[ip->i_hlink])
                if (ino == ip->i_number && dev == ip->i_dev)
                        return (ip);
        return ((struct inode *)0);
}

/*
 * Look up an inode by device,inumber,fstyp.
 * If it is in core (in the inode structure),  honor the locking protocol.
 * If it is not in core, read it in from the specified device.
 * If the inode is mounted on, perform the indicated indirection.
 * In all cases, a pointer to a locked inode structure is returned.
 *
 * panic: no imt -- if the mounted file system is not in the mount table."cannot happen"
 */
struct inode *iget(dev_t dev, ino_t ino, int fstyp)
{
        struct inode *ip;
        struct mount *mp;
        struct buf *bp;
        struct dinode *dp;
        int     slot;
loop:
        slot = INOHASH(dev, ino, fstyp);
        ip = &inode[inohash[slot]];
        while (ip != &inode[-1]) {
                if (ino == ip->i_number && dev == ip->i_dev && fstyp == ip->i_fstyp) {
                        if ((ip->i_flag & ILOCK) != 0) {
                                ip->i_flag |= IWANT;
                                sleep((caddr_t)ip, PINOD);
                                goto loop;
                        }
                        if ((ip->i_flag & IMOUNT) != 0) {
                                for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
                                        if (mp->m_inodp == ip) {
                                                dev = mp->m_dev;
                                                ino = ROOTINO;
                                                fstyp = mp->m_fstyp;
                                                goto loop;
                                        }
                                panic("no imt");
                        }
                        ip->i_count++;
                        ip->i_flag |= ILOCK;
                        return (ip);
                }
                ip = &inode[ip->i_hlink];
        }
        if (ifreel < 0) {
                tablefull("inode");
                u.u_error = ENFILE;
                return (NULL);
        }
        ip = &inode[ifreel];
        ifreel = ip->i_hlink;
        ip->i_hlink = inohash[slot];
        inohash[slot] = ip - inode;
        ip->i_dev = dev;
        ip->i_fstyp = fstyp;
        ip->i_number = ino;
        ip->i_flag = ILOCK;
        ip->i_count++;
        ip->i_sptr = NULL;
        if (fstyp && fstypsw[fstyp].t_get)
                return ((*fstypsw[fstyp].t_get)(fstyp, dev, ino, ip));
        ip->i_un.i_lastr = 0;
        bp = bread(dev, itod(dev, ino));
        /*
         * check I/O errors
         */
        if ((bp->b_flags & B_ERROR) != 0) {
                brelse(bp);
                iput(ip);
                return (NULL);
        }
        dp = bp->b_un.b_dino;
        dp += itoo(dev, ino);
        iexpand(ip, dp);
        brelse(bp);
        return (ip);
}
