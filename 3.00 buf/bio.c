#include "buf.h"
/*
 * The following routines allocate a buffer:
 *	getblk
 *	bread
 *	breada
 *	baddr	(if it is incore)
 * Eventually the buffer must be released, possibly with the
 * side effect of writing it out, by using one of
 *	bwrite
 *	bdwrite
 *	bawrite
 *	brelse
 */

struct buf bfreelist[BQUEUES];
struct buf bswlist, *bclnlist;

#define BUFHSZ  63
struct bufhd bufhash[BUFHSZ];
#define BUFHASH(dev, dblkno) \
                ((struct buf *) &bufhash[((int)(dev) + (int)(dblkno)) % BUFHSZ])
/*
 * Initialize hash links for buffers.
 */
void bhinit()
{
        int     i;
        struct bufhd *bp;
        
        for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
                bp->b_forw = bp->b_back = (struct buf *) bp;
}
/*
 * notavail(bp):
 * remove buf block from freelist, and
 * mark it busy.
 */
#define notavail(bp) \
{ \
        (bp)->av_back->av_forw = (bp)->av_forw; \
        (bp)->av_forw->av_back = (bp)->av_back; \
        (bp)->b_flags |= B_BUSY;                \
}

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 */
struct buf *getblk(dev_t dev; daddr_t blkno)
{
        struct buf *bp, *dp, *ep;
        int     dblkno = fsbtodb(dev, blkno);
        
        if ((unsigned)blkno >= 1 << (sizeof(int) * NBBY - PGSHIFT))
                blkno = 1 << ((sizeof(int) *NBBY - PGSHIFT) + 1);
        dblkno = fsbtodb(dev, blkno);
        dp = BUFHASH(dev, dblkno);
loop:
        for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
                if (bp->b_blkno != dblkno || bp->b_dev != dev ||
                                bp->b_flags & B_INVAL)
                        continue;
                if (bp->b_flags & B_BUSY) {
                        bp->b_flags |= B_WANTED;
                        sleep((caddr_t)bp, PRIBIO+1);
                        goto loop;
                }
                notavail(bp);
                bp->b_flags |= B_CACHE;
                return (bp);
        }
        if (major(dev) >= nblkdev)      /* max block device ? */
                panic("blkdev");        /* error message */
        for (ep = &bfreelist[BQUEUES-1]; ep > bfreelist; ep--)
                if (ep->av_forw != ep)
                        break;
        if (ep == bfreelist) {          /* no free blocks at all */
                ep->b_flags |= B_WANTED;
                sleep((caddr_t)ep, PRIBIO+1);
                goto loop;
        }
        bp = ep->av_forw;
        notavail(bp);
        if (bp->b_flags & B_DELWRI) {
                bp->b_flags |= B_ASYNC;
                bwrite(bp);
                goto loop;
        }
        bp->b_flags = B_BUSY;
        bp->b_back->b_forw = bp->b_forw;
        bp->b_forw->b_back = bp->b_back;
        bp->b_forw = dp->b_forw;
        bp->b_back = dp;
        dp->b_forw->b_back = bp;
        dp->b_forw = bp;
        bp->b_dev = dev;
        bp->b_blkno = dblkno;
        return (bp);
}

/*
 * get an empty block,
 * not assigned to any particular device
 */
struct buf *geteblk()
{
        struct buf *bp, *dp;
        int     s;
        
loop:
        for (dp = &bfreelist[BQUEUES-1]; dp > bfreelist; dp--)
                if (dp->av_forw != dp)
                        break;
        if (dp == bfreelist) {          /* no free blocks */
                dp->b_flags |= B_WANTED;
                sleep((caddr_t)dp, PRIBIO+1);
                goto loop;
        }
        bp = dp->av_forw;
        notavail(bp);
        if (bp->b_flags & B_DELWRI) {
                bp->b_flags |= B_ASYNC;
                bwrite(bp);
                goto loop;
        }
        bp->b_flags = B_BUSY | B_INVAL;
        bp->b_back->b_forw = bp->b_forw;
        bp->b_forw->b_back = bp->b_back;
        bp->b_forw = dp->b_forw;
        bp->b_back = dp;
        bp->b_forw->b_back = bp;
        dp->b_forw = bp;
        bp->b_dev = (dev_t) NODEV;
        return (bp);
}




