typedef unsigned char u_char;
typedef unsigned short u_short;

typedef char * caddr_t;
typedef long    daddr_t;
typedef u_short ino_t;
typedef u_short dev_t;
typedef long    off_t;

#define NADDR   13

struct inode {
        short   i_flag;
        u_char  i_count;        /* reference count */
        char    i_fstyp;       /* type of its filesystem */
        dev_t   i_dev;          /* device where inode resides */
        long    i_number;       /* i number, 1-to-1 with device address */
        u_short i_mode;
        short   i_nlink;        /* directory entries */
        short   i_uid;          /* owner */
        short   i_gid;          /* group of owner */
        off_t   i_size;         /* size of file */
        struct stdata *i_sptr;  /* stream associated with this inode */
        union {
                struct {
                        daddr_t I_addr[NADDR];  /* if normal file/directory */
                        daddr_t I_lastr;        /* last read (for read-ahead) */
                } i_f;
#define i_addr  i_f.I_addr
#define i_lastr i_f.I_lastr
                struct {
                        daddr_t I_rdev;         /* i_addr[0] */
                        long    I_key;          /* see istread */
                } i_d;
#define i_rdev  i_d.I_rdev
#define i_key   i_d.I_key
                struct {
                        long    I_tag;
                        struct inode *I_cip;    /* communications */
                } i_a;                          /* when i_fstype != 0 */
#define i_tag   i_a.I_tag
#define i_cip   i_a.I_cip
                struct {
                        struct proc *I_proc;    /* sanity checking */
                        int     I_sigmask;      /* signal trace mask */
                } i_p;
#define i_proc  i_p.I_proc
#define i_sigmask       i_p.I_sigmask                
        } i_un;
        short   i_hlink;                /* link in hash chain (iget/iput/ifind) */
};

/* flags */
#define	ILOCK	01		/* inode is locked */
#define	IUPD	02		/* file has been modified */
#define	IACC	04		/* inode access time to be updated */
#define	IMOUNT	010		/* inode is mounted on */
#define	IWANT	020		/* some process waiting on lock */
#define	ITEXT	040		/* inode is pure text prototype */
#define	ICHG	0100		/* inode has been changed */
#define	IPIPE	0200		/* inode is a pipe */
#define IDEAD	0400		/* only iput and iget */
#define	IALOCK	01000		/* advisory lock has been set */

/* modes */
#define	IFMT	0170000		/* type of file */
#define	IFDIR	0040000	        /* directory */
#define	IFCHR	0020000	        /* character special */
#define	IFBLK	0060000	        /* block special */
#define	IFREG	0100000	        /* regular */
#define	IFLNK	0120000         /* symbolic link to another file */
#define	ISUID	04000		/* set user id on execution */
#define	ISGID	02000		/* set group id on execution */
#define	ISVTX	01000		/* save swapped text even after use */
#define	IREAD	0400		/* read, write, execute permissions */
#define	IWRITE	0200
#define	IEXEC	0100

struct argnamei {               /* namei's flag argument */
        short   flag;           /* type of request */
        ino_t   ino;            /* ino and dev for linking */
        dev_t   idev;
        short   mode;           /* mode for creating */
};
#define NI_DEL	1	/* unlink this file */
#define NI_CREAT 2	/* create it if it doesn't exits */
#define NI_NXCREAT 3	/* create it, error if it already exists */
#define NI_LINK	4	/* make a link */
#define NI_MKDIR 5	/* make a directory */
#define NI_RMDIR 6	/* remove a directory */
struct nx {     /* arg to real nami */
        struct inode *dp;
        char    *cp;
        struct buf *nbp;
        int     nlink;
};

typedef long    time_t;
struct dinode {
        u_short di_mode;        /* mode and type of file */
        short   di_nlink;       /* number of links to file */
        short   di_uid;
        short   di_gid;
        off_t   di_size;
        char    di_addr[40];    /* 39 used; 13 addresses of 3 bytes each. */
        time_t  di_atime;       /* time last accessed */
        time_t  di_mtime;       /* time last modified */
        time_t  di_ctime;       /* time created */        
};