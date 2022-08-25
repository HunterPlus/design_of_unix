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




