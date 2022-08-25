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




