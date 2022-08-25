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
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 */
int incore(dev_t dev, daddr_t blkno)
{
        struct buf *bp, *dp;
        int     dblkno = fsbtodb(dev, blkno);
        
        dp = BUFHASH(dev, dblkno);
        for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
                if (bp->b_blkno == dblkno && bp->b_dev == dev &&
                                !(bp->b_flags & B_INVAL))
                        return (1);
        return (0);
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 */
struct buf *breada(dev_t dev, daddr_t blkno, daddr_t rablkno)
{
        struct buf *bp, *rabp;
        
        bp = NULL;
        if (!incore(dev, blkno)) {
                bp = getblk(dev, blkno);
                if ((bp->b_flags & B_DONE) == 0) {
                        bp->b_flags |= B_READ;
                        bp->b_bcount = BSIZE(dev);
                        (*bdevsw[major(dev)].d_strategy)(bp);
                        u.u_vm.vm_inblk++;
                }
        }
        if (rablkno && !incore(dev, rablkno)) {
                rabp = getblk(dev, rablkno);
                if (rabp->b_flags & B_DONE) {
                        brelse(rabp);
                } else {
                        rabp->b_flags |= B_READ | B_ASYNC;
                        rabp->b_bcount = BSIZE(dev);
                        (*bdevsw[jajor(dev)].d_strategy)(rabp);
                        u.u_vm.vm_inblk++;
                }
        }
        if (bp == NULL)
                return bread(dev, blkno);
        iowait(bp);
        return (bp);
}

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *bread(dev_t dev, daddr_t blkno)
{
        struct buf *bp;
        
        bp = getblk(dev, blkno);
        if (bp->b_flags & B_DONE) 
                return bp;
        bp->b_flags |= B_READ;
        bp->b_bcount = BSIZE(dev);
        (*bdevsw[major(dev)].d_strategy)(bp);
        u.u_vm.vm_inblk++;      /* pay for read */
        iowait(bp);
        return (bp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
void bawrite(struct buf *bp)
{
        bp->b_flags |= B_ASYNC;
        bwrite(bp);
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
void bwrite(struct buf *bp)
{
        int     flag;
        
        flag = bp->b_flags;
        bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI | B_AGE);
        bp->b_bcount = BSIZE(bp->b_dev);
        if ((flag & B_DELWRI) == 0)
                u.u_vm.vm_oublk++;      /* noone paid yet */
        (*bdevsw[major(bp->b_dev)].d_strategy)(bp);     /* perform I/O for internal functions */
        if ((flag & B_ASYNC) == 0) {
                iowait(bp);
                brelse(bp);
        } else if (flag & B_DELWRI)
                bp->b_flags |= B_AGE;
        else
                geterror(bp);
}

/*
 * release the buffer, with no I/O implied.
 */
void brelse(struct buf *bp)
{
        struct buf *flist;
        int     s;
        
        if (bp->b_flags & B_WANTED)
                wakeup((caddr_t)bp);    /* someone sleep on this event */
        if (bfreelist[0].b_flags & B_WANTED) {  /* locked freelist */
              bfreelist[0].b_flags &= ~B_WANTED;
              wakeup((caddr_t)bfreelist);
        }
        if (bp->b_flags & B_ERROR)
                if (bp->b_flags & B_LOCKED)
                        bp->b_flags &= ~B_ERROR;
                else
                        bp->b_dev = NODEV;
        if (bp->b_flags & (B_ERROR | B_INVAL)) {
                /* block has no info ... put at front of most free list */
                flist = &bfreelist[BQUEUE - 1];
                flist->av_forw->av_back = bp;
                bp->av_forw = flist->av_forw;
                flist->av_forw = bp;
                bp->av_back = flist;
        } else {
                if (bp->b_flags & B_LOCKED)
                        flist = &bfreelist[BQ_LOCKED];
                else if (bp->b_flags & B_AGE)
                        flist = &bfreelist[BQ_AGE];
                else
                        flist = &bfreelist[BQ_LRU];
                flist->av_back->av_forw = bp;
                bp->av_back = flist->av_back;
                flist->av_back = bp;
                bp->av_forw = flist;
        }
        bp->b_flags &= ~(B_WANTED | B_BUSY | B_ASYNC | B_AGE);
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




