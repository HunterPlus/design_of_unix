/*
 * There are currently three queues for buffers:
 *	one for buffers which must be kept permanently (super blocks)
 * 	one for buffers containing ``useful'' information (the cache)
 *	one for buffers containing ``non-useful'' information
 *		(and empty buffers, pushed onto the front)
 */
typedef int     dev_t;
typedef char *  caddr_t;                  /* legacy */
typedef long    daddr_t;

struct bufhd {
        long    b_flags;        
        struct buf *b_forw, *b_back;
};
struct buf {
        long    b_flags;
        struct buf *b_forw, *b_back;    /* hash chain (2 way street) */
        struct buf *av_forw, *av_back;  /* position on free list if not BUSY */

        long    b_bcount;                /* transfer count */
        short   b_error;                /* returned after I/O */
        dev_t   b_dev;                  /* major+minor device name */
        union {
                caddr_t b_addr;         /* low order core address */
                int     *b_words;       /* words for clearing */
                struct filsys *b_filsys;        /* superblocks */
                struct dinode *b_dino;          /* ilist */
                daddr_t *b_daddr;       /* indirect block */
        } b_un;
        daddr_t b_blkno;                /* block # on device */
        long    b_resid;                /* words not transferred after error */
        struct proc *b_proc;            /* proc doing physical or swap I/O */
};

#define BQUEUES         3               /* number of free buffer queues */
#define BQ_LOCKED       0               /* super-blocks &c */
#define BQ_LRU          1               /* lru, useful buffers */
#define BQ_AGE          2               /* rubbish */

/*
 * These flags are kept in b_flags.
 */
#define	B_WRITE		0x000000	/* non-read pseudo-flag */
#define	B_READ		0x000001	/* read when I/O occurs */
#define	B_DONE		0x000002	/* transaction finished */
#define	B_ERROR		0x000004	/* transaction aborted */
#define	B_BUSY		0x000008	/* not on av_forw/back list */
#define	B_PHYS		0x000010	/* physical IO */
#define	B_HDR		0x000020	/* was B_MAP, now ioctl */
#define	B_WANTED	0x000040	/* issue wakeup when BUSY goes off */
#define	B_AGE		0x000080	/* delayed write for correct aging */
#define	B_ASYNC		0x000100	/* don't wait for I/O completion */
#define	B_DELWRI	0x000200	/* write at exit of avail list */
#define	B_TAPE		0x000400	/* this is a magtape (no bdwrite) */
#define	B_UAREA		0x000800	/* add u-area to a swap operation */
#define	B_PAGET		0x001000	/* page in/out of page table space */
#define	B_DIRTY		0x002000	/* dirty page to be pushed out async */
#define	B_PGIN		0x004000	/* pagein op, so swap() can count it */
#define	B_CACHE		0x008000	/* did bread find us in the cache ? */
#define	B_INVAL		0x010000	/* does not contain valid info  */
#define	B_LOCKED	0x020000	/* locked in core (not reusable) */
#define	B_HEAD		0x040000	/* a buffer header, not a buffer */
#define	B_BAD		0x100000	/* bad block revectoring in progress */