#include "buf.h"
#include "inode.h"

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead.
 */
daddr_t bmap(struct inode *ip, daddr_t bn, int rwflg)
{
        int     i;
        struct buf *bp, *nbp;
        int     j, sh, prev;
        daddr_t nb, *bap;
        dev_t   dev;

        if (bn < 0) {
                u.u_error = EFBIG;
                return ((daddr_t)0);
        }
        dev = ip->i_dev;
        rablock = 0;

        /*
         * blocks 0..NADDR-4 are direct blocks
         */
        if (bn < NADDR-3) {
                i = bn;
                nb = ip->i_un.i_addr[i];
                if (nb == 0) {
                        prev = (i > 0? ip->i_un.i_addr[i-1] : 0);
                        if (rwflg == B_READ || (bp = alloc(dev, prev)) == NULL)
                                return ((daddr_t)-1);
                        nb = dbtofsb(dev, bp->b_blkno);
                        if ((ip->i_mode & IFMT) == IFDIR)
                                /*
                                 * Write directory blocks synchronously
                                 * so they never appear with garbage in them on the disk.
                                 */
                                bwrite(bp);
                        else
                                bdwrite(bp);
                        ip->i_un.i_addr[i] = nb;
                        ip->i_flag |= IUPD | ICHG;
                }
                if (i < NADDR-4)
                        rablock = ip->i_un.i_addr[i+1];
                return (nb);
        }
	/*
	 * addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
        sh = 0;
        nb = 1;
        bn -= NADDR-3;
        for (j = 3; j > 0; j--) {
                sh += NSHIFT(dev);
                nb <<= NSHIFT(dev);
                if (bn < nb)
                        break;
                bn -= nb;
        }
        if (j = 0) {
                u.u_error = EFBIG;
                return ((daddr_t)0);
        }
	/*
	 * fetch the first indirect block
	 */

        
}