/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This header file defines the interface that the sqlite B-Tree file
** subsystem.  See comments in the source code for a detailed description
** of what each interface routine does.
**
** SQLite B-Tree 存储引擎接口头文件
**
** 本文件定义了 SQLite B-Tree 文件子系统的接口。B-Tree 是 SQLite 的核心存储引擎，
** 负责管理数据库文件的页式存储结构，包括表的 B+Tree 索引和索引的 B-Tree 结构。
**
** 主要功能：
** 1. B-Tree 数据库文件的打开、关闭和配置管理
** 2. 事务控制：开始、提交、回滚和保存点操作
** 3. 表和索引的创建、删除和管理
** 4. 游标操作：数据的插入、查询、更新和删除
** 5. 页面管理：缓存、锁定和并发控制
** 6. 自动清理和空间回收机制
**
** 设计特点：
** - 页式存储：固定大小的页面（通常 4KB）作为基本 I/O 单位
** - B+Tree 结构：叶子节点包含实际数据，内部节点用于索引
** - 事务 ACID：通过回滚日志和 WAL 模式保证事务特性
** - 并发控制：多粒度锁定机制支持多用户并发访问
** - 崩溃恢复：重做日志和检查点机制保证数据一致性
**
** 使用说明：
** - 本接口是 SQLite 存储层的核心抽象，供上层 VDBE 调用
** - 应用程序通常不直接使用这些接口，而是通过标准 SQL API
** - 理解 B-Tree 接口有助于深入分析 SQLite 的存储和性能特性
*/
#ifndef SQLITE_BTREE_H
#define SQLITE_BTREE_H

/* TODO: This definition is just included so other modules compile. It
** needs to be revisited.
**
** TODO：此定义仅为了让其他模块能够编译而包含，需要重新审查。
*/
#define SQLITE_N_BTREE_META 16         /* B-Tree 元数据页面中的字段数量 */

/*
** If defined as non-zero, auto-vacuum is enabled by default. Otherwise
** it must be turned on for each database using "PRAGMA auto_vacuum = 1".
**
** 如果定义为非零值，默认启用自动清理。否则必须对每个数据库使用
** "PRAGMA auto_vacuum = 1" 来启用。
**
** 自动清理（Auto-Vacuum）功能：
** - 当删除大量数据时，自动收缩数据库文件大小
** - FULL 模式：每次事务后自动清理，但会增加事务开销
** - INCR 模式：增量清理，需要手动执行 VACUUM 命令
** - NONE 模式：不自动清理，文件大小可能增长但不会收缩
*/
#ifndef SQLITE_DEFAULT_AUTOVACUUM
  #define SQLITE_DEFAULT_AUTOVACUUM 0  /* 默认不启用自动清理 */
#endif

/* B-Tree 自动清理模式定义 */
#define BTREE_AUTOVACUUM_NONE 0        /* Do not do auto-vacuum - 不执行自动清理 */
#define BTREE_AUTOVACUUM_FULL 1        /* Do full auto-vacuum - 执行完整自动清理 */
#define BTREE_AUTOVACUUM_INCR 2        /* Incremental vacuum - 增量清理模式 */

/*
** Forward declarations of structure
**
** 结构体前向声明 - 定义 B-Tree 子系统的核心数据结构
**
** 这些是 B-Tree 存储引擎的主要数据结构：
**
** Btree: 表示一个打开的 B-Tree 数据库连接
**        - 包含数据库文件引用、配置信息和事务状态
**        - 每个数据库连接可以包含多个 B-Tree 实例（main、temp、附加数据库）
**
** BtCursor: B-Tree 游标，用于在 B-Tree 中导航和操作数据
**           - 指向 B-Tree 中的特定位置（表或索引中的某行）
**           - 支持插入、查询、更新、删除操作
**           - 维护游标状态和缓存信息
**
** BtShared: 共享的 B-Tree 信息，支持多个连接共享同一个数据库文件
**           - 包含页面缓存、锁管理器和 Schema 信息
**           - 实现连接间的数据共享和并发控制
**           - 管理数据库文件的物理结构
**
** BtreePayload: B-Tree 负载数据结构
**              - 表示要存储在 B-Tree 节点中的数据
**              - 包含键值对、数据长度和实际内容
**              - 用于插入和更新操作时的数据传递
*/
typedef struct Btree Btree;              /* B-Tree 数据库连接结构 */
typedef struct BtCursor BtCursor;        /* B-Tree 游标结构 */
typedef struct BtShared BtShared;        /* 共享 B-Tree 信息结构 */
typedef struct BtreePayload BtreePayload; /* B-Tree 负载数据结构 */


/*
** 打开 B-Tree 数据库连接
**
** 此函数创建一个新的 B-Tree 数据库连接，用于访问 SQLite 数据库文件。
** 这是 SQLite 存储引擎的入口点，建立了从内存结构到磁盘文件的桥梁。
**
** 参数说明：
**
** pVfs: 要使用的虚拟文件系统接口
**      - 定义文件 I/O、路径操作、线程同步等系统调用抽象
**      - 支持加密、压缩等文件系统扩展功能
**      - 通常使用 sqlite3_vfs_find() 获取默认 VFS
**
** zFilename: 要打开的数据库文件名
**           - 对于内存数据库使用 ":memory:"
**           - 对于临时数据库使用 "" 或 NULL
**           - 可以包含相对路径或绝对路径
**           - 支持 UTF-8 编码的文件名
**
** db: 关联的数据库连接
**    - 指向包含此 B-Tree 的 sqlite3 连接结构
**    - 用于错误报告和内存分配
**    - 共享连接配置和状态信息
**
** ppBtree: 返回打开的 Btree* 指针的地址
**         - 成功时指向新创建的 Btree 结构
**         - 失败时保持不变，调用者应检查返回值
**         - 后续 B-Tree 操作都使用此指针
**
** flags: B-Tree 操作标志位（可以是以下值的按位或）：
**       - BTREE_OMIT_JOURNAL: 不创建或使用回滚日志
**       - BTREE_MEMORY: 内存数据库，不访问磁盘
**       - BTREE_SINGLE: 文件最多包含一个 B-Tree
**       - BTREE_UNORDERED: 可以使用哈希实现
**
** vfsFlags: 传递给 VFS 打开函数的标志
**          - 包含文件打开模式、共享模式等
**          - 由 B-Tree 模块根据 flags 参数计算得出
**
** 返回值：SQLITE_OK 表示成功，其他值表示错误代码
*/
int sqlite3BtreeOpen(
  sqlite3_vfs *pVfs,       /* VFS to use with this b-tree - 虚拟文件系统接口 */
  const char *zFilename,   /* Name of database file to open - 数据库文件名 */
  sqlite3 *db,             /* Associated database connection - 关联的数据库连接 */
  Btree **ppBtree,         /* Return open Btree* here - 返回的 Btree 指针 */
  int flags,               /* Flags - B-Tree 操作标志 */
  int vfsFlags             /* Flags passed through to VFS open - VFS 打开标志 */
);

/* The flags parameter to sqlite3BtreeOpen can be the bitwise or of the
** following values.
**
** NOTE:  These values must match the corresponding PAGER_ values in
** pager.h.
*/
#define BTREE_OMIT_JOURNAL  1  /* Do not create or use a rollback journal */
#define BTREE_MEMORY        2  /* This is an in-memory DB */
#define BTREE_SINGLE        4  /* The file contains at most 1 b-tree */
#define BTREE_UNORDERED     8  /* Use of a hash implementation is OK */

int sqlite3BtreeClose(Btree*);
int sqlite3BtreeSetCacheSize(Btree*,int);
int sqlite3BtreeSetSpillSize(Btree*,int);
#if SQLITE_MAX_MMAP_SIZE>0
  int sqlite3BtreeSetMmapLimit(Btree*,sqlite3_int64);
#endif
int sqlite3BtreeSetPagerFlags(Btree*,unsigned);
int sqlite3BtreeSetPageSize(Btree *p, int nPagesize, int nReserve, int eFix);
int sqlite3BtreeGetPageSize(Btree*);
Pgno sqlite3BtreeMaxPageCount(Btree*,Pgno);
Pgno sqlite3BtreeLastPage(Btree*);
int sqlite3BtreeSecureDelete(Btree*,int);
int sqlite3BtreeGetRequestedReserve(Btree*);
int sqlite3BtreeGetReserveNoMutex(Btree *p);
int sqlite3BtreeSetAutoVacuum(Btree *, int);
int sqlite3BtreeGetAutoVacuum(Btree *);
int sqlite3BtreeBeginTrans(Btree*,int,int*);
int sqlite3BtreeCommitPhaseOne(Btree*, const char*);
int sqlite3BtreeCommitPhaseTwo(Btree*, int);
int sqlite3BtreeCommit(Btree*);
int sqlite3BtreeRollback(Btree*,int,int);
int sqlite3BtreeBeginStmt(Btree*,int);
int sqlite3BtreeCreateTable(Btree*, Pgno*, int flags);
int sqlite3BtreeTxnState(Btree*);
int sqlite3BtreeIsInBackup(Btree*);

void *sqlite3BtreeSchema(Btree *, int, void(*)(void *));
int sqlite3BtreeSchemaLocked(Btree *pBtree);
#ifndef SQLITE_OMIT_SHARED_CACHE
int sqlite3BtreeLockTable(Btree *pBtree, int iTab, u8 isWriteLock);
#endif

/* Savepoints are named, nestable SQL transactions mostly implemented */ 
/* in vdbe.c and pager.c See https://sqlite.org/lang_savepoint.html */
int sqlite3BtreeSavepoint(Btree *, int, int);

/* "Checkpoint" only refers to WAL. See https://sqlite.org/wal.html#ckpt */
#ifndef SQLITE_OMIT_WAL
  int sqlite3BtreeCheckpoint(Btree*, int, int *, int *);  
#endif

const char *sqlite3BtreeGetFilename(Btree *);
const char *sqlite3BtreeGetJournalname(Btree *);
int sqlite3BtreeCopyFile(Btree *, Btree *);

int sqlite3BtreeIncrVacuum(Btree *);

/* The flags parameter to sqlite3BtreeCreateTable can be the bitwise OR
** of the flags shown below.
**
** Every SQLite table must have either BTREE_INTKEY or BTREE_BLOBKEY set.
** With BTREE_INTKEY, the table key is a 64-bit integer and arbitrary data
** is stored in the leaves.  (BTREE_INTKEY is used for SQL tables.)  With
** BTREE_BLOBKEY, the key is an arbitrary BLOB and no content is stored
** anywhere - the key is the content.  (BTREE_BLOBKEY is used for SQL
** indices.)
*/
#define BTREE_INTKEY     1    /* Table has only 64-bit signed integer keys */
#define BTREE_BLOBKEY    2    /* Table has keys only - no data */

int sqlite3BtreeDropTable(Btree*, int, int*);
int sqlite3BtreeClearTable(Btree*, int, i64*);
int sqlite3BtreeClearTableOfCursor(BtCursor*);
int sqlite3BtreeTripAllCursors(Btree*, int, int);

void sqlite3BtreeGetMeta(Btree *pBtree, int idx, u32 *pValue);
int sqlite3BtreeUpdateMeta(Btree*, int idx, u32 value);

int sqlite3BtreeNewDb(Btree *p);

/*
** The second parameter to sqlite3BtreeGetMeta or sqlite3BtreeUpdateMeta
** should be one of the following values. The integer values are assigned 
** to constants so that the offset of the corresponding field in an
** SQLite database header may be found using the following formula:
**
**   offset = 36 + (idx * 4)
**
** For example, the free-page-count field is located at byte offset 36 of
** the database file header. The incr-vacuum-flag field is located at
** byte offset 64 (== 36+4*7).
**
** The BTREE_DATA_VERSION value is not really a value stored in the header.
** It is a read-only number computed by the pager.  But we merge it with
** the header value access routines since its access pattern is the same.
** Call it a "virtual meta value".
*/
#define BTREE_FREE_PAGE_COUNT     0
#define BTREE_SCHEMA_VERSION      1
#define BTREE_FILE_FORMAT         2
#define BTREE_DEFAULT_CACHE_SIZE  3
#define BTREE_LARGEST_ROOT_PAGE   4
#define BTREE_TEXT_ENCODING       5
#define BTREE_USER_VERSION        6
#define BTREE_INCR_VACUUM         7
#define BTREE_APPLICATION_ID      8
#define BTREE_DATA_VERSION        15  /* A virtual meta-value */

/*
** Kinds of hints that can be passed into the sqlite3BtreeCursorHint()
** interface.
**
** BTREE_HINT_RANGE  (arguments: Expr*, Mem*)
**
**     The first argument is an Expr* (which is guaranteed to be constant for
**     the lifetime of the cursor) that defines constraints on which rows
**     might be fetched with this cursor.  The Expr* tree may contain
**     TK_REGISTER nodes that refer to values stored in the array of registers
**     passed as the second parameter.  In other words, if Expr.op==TK_REGISTER
**     then the value of the node is the value in Mem[pExpr.iTable].  Any
**     TK_COLUMN node in the expression tree refers to the Expr.iColumn-th
**     column of the b-tree of the cursor.  The Expr tree will not contain
**     any function calls nor subqueries nor references to b-trees other than
**     the cursor being hinted.
**
**     The design of the _RANGE hint is aid b-tree implementations that try
**     to prefetch content from remote machines - to provide those
**     implementations with limits on what needs to be prefetched and thereby
**     reduce network bandwidth.
**
** Note that BTREE_HINT_FLAGS with BTREE_BULKLOAD is the only hint used by
** standard SQLite.  The other hints are provided for extensions that use
** the SQLite parser and code generator but substitute their own storage
** engine.
*/
#define BTREE_HINT_RANGE 0       /* Range constraints on queries */

/*
** Values that may be OR'd together to form the argument to the
** BTREE_HINT_FLAGS hint for sqlite3BtreeCursorHint():
**
** The BTREE_BULKLOAD flag is set on index cursors when the index is going
** to be filled with content that is already in sorted order.
**
** The BTREE_SEEK_EQ flag is set on cursors that will get OP_SeekGE or
** OP_SeekLE opcodes for a range search, but where the range of entries
** selected will all have the same key.  In other words, the cursor will
** be used only for equality key searches.
**
*/
#define BTREE_BULKLOAD 0x00000001  /* Used to full index in sorted order */
#define BTREE_SEEK_EQ  0x00000002  /* EQ seeks only - no range seeks */

/* 
** Flags passed as the third argument to sqlite3BtreeCursor().
**
** For read-only cursors the wrFlag argument is always zero. For read-write
** cursors it may be set to either (BTREE_WRCSR|BTREE_FORDELETE) or just
** (BTREE_WRCSR). If the BTREE_FORDELETE bit is set, then the cursor will
** only be used by SQLite for the following:
**
**   * to seek to and then delete specific entries, and/or
**
**   * to read values that will be used to create keys that other
**     BTREE_FORDELETE cursors will seek to and delete.
**
** The BTREE_FORDELETE flag is an optimization hint.  It is not used by
** by this, the native b-tree engine of SQLite, but it is available to
** alternative storage engines that might be substituted in place of this
** b-tree system.  For alternative storage engines in which a delete of
** the main table row automatically deletes corresponding index rows,
** the FORDELETE flag hint allows those alternative storage engines to
** skip a lot of work.  Namely:  FORDELETE cursors may treat all SEEK
** and DELETE operations as no-ops, and any READ operation against a
** FORDELETE cursor may return a null row: 0x01 0x00.
*/
#define BTREE_WRCSR     0x00000004     /* read-write cursor */
#define BTREE_FORDELETE 0x00000008     /* Cursor is for seek/delete only */

int sqlite3BtreeCursor(
  Btree*,                              /* BTree containing table to open */
  Pgno iTable,                         /* Index of root page */
  int wrFlag,                          /* 1 for writing.  0 for read-only */
  struct KeyInfo*,                     /* First argument to compare function */
  BtCursor *pCursor                    /* Space to write cursor structure */
);
BtCursor *sqlite3BtreeFakeValidCursor(void);
int sqlite3BtreeCursorSize(void);
#ifdef SQLITE_DEBUG
int sqlite3BtreeClosesWithCursor(Btree*,BtCursor*);
#endif
void sqlite3BtreeCursorZero(BtCursor*);
void sqlite3BtreeCursorHintFlags(BtCursor*, unsigned);
#ifdef SQLITE_ENABLE_CURSOR_HINTS
void sqlite3BtreeCursorHint(BtCursor*, int, ...);
#endif

int sqlite3BtreeCloseCursor(BtCursor*);
int sqlite3BtreeTableMoveto(
  BtCursor*,
  i64 intKey,
  int bias,
  int *pRes
);
int sqlite3BtreeIndexMoveto(
  BtCursor*,
  UnpackedRecord *pUnKey,
  int *pRes
);
int sqlite3BtreeCursorHasMoved(BtCursor*);
int sqlite3BtreeCursorRestore(BtCursor*, int*);
int sqlite3BtreeDelete(BtCursor*, u8 flags);

/* Allowed flags for sqlite3BtreeDelete() and sqlite3BtreeInsert() */
#define BTREE_SAVEPOSITION 0x02  /* Leave cursor pointing at NEXT or PREV */
#define BTREE_AUXDELETE    0x04  /* not the primary delete operation */
#define BTREE_APPEND       0x08  /* Insert is likely an append */
#define BTREE_PREFORMAT    0x80  /* Inserted data is a preformated cell */

/* An instance of the BtreePayload object describes the content of a single
** entry in either an index or table btree.
**
** Index btrees (used for indexes and also WITHOUT ROWID tables) contain
** an arbitrary key and no data.  These btrees have pKey,nKey set to the
** key and the pData,nData,nZero fields are uninitialized.  The aMem,nMem
** fields give an array of Mem objects that are a decomposition of the key.
** The nMem field might be zero, indicating that no decomposition is available.
**
** Table btrees (used for rowid tables) contain an integer rowid used as
** the key and passed in the nKey field.  The pKey field is zero.  
** pData,nData hold the content of the new entry.  nZero extra zero bytes
** are appended to the end of the content when constructing the entry.
** The aMem,nMem fields are uninitialized for table btrees.
**
** Field usage summary:
**
**               Table BTrees                   Index Btrees
**
**   pKey        always NULL                    encoded key
**   nKey        the ROWID                      length of pKey
**   pData       data                           not used
**   aMem        not used                       decomposed key value
**   nMem        not used                       entries in aMem
**   nData       length of pData                not used
**   nZero       extra zeros after pData        not used
**
** This object is used to pass information into sqlite3BtreeInsert().  The
** same information used to be passed as five separate parameters.  But placing
** the information into this object helps to keep the interface more 
** organized and understandable, and it also helps the resulting code to
** run a little faster by using fewer registers for parameter passing.
*/
struct BtreePayload {
  const void *pKey;       /* Key content for indexes.  NULL for tables */
  sqlite3_int64 nKey;     /* Size of pKey for indexes.  PRIMARY KEY for tabs */
  const void *pData;      /* Data for tables. */
  sqlite3_value *aMem;    /* First of nMem value in the unpacked pKey */
  u16 nMem;               /* Number of aMem[] value.  Might be zero */
  int nData;              /* Size of pData.  0 if none. */
  int nZero;              /* Extra zero data appended after pData,nData */
};

int sqlite3BtreeInsert(BtCursor*, const BtreePayload *pPayload,
                       int flags, int seekResult);
int sqlite3BtreeFirst(BtCursor*, int *pRes);
int sqlite3BtreeIsEmpty(BtCursor *pCur, int *pRes);
int sqlite3BtreeLast(BtCursor*, int *pRes);
int sqlite3BtreeNext(BtCursor*, int flags);
int sqlite3BtreeEof(BtCursor*);
int sqlite3BtreePrevious(BtCursor*, int flags);
i64 sqlite3BtreeIntegerKey(BtCursor*);
void sqlite3BtreeCursorPin(BtCursor*);
void sqlite3BtreeCursorUnpin(BtCursor*);
i64 sqlite3BtreeOffset(BtCursor*);
int sqlite3BtreePayload(BtCursor*, u32 offset, u32 amt, void*);
const void *sqlite3BtreePayloadFetch(BtCursor*, u32 *pAmt);
u32 sqlite3BtreePayloadSize(BtCursor*);
sqlite3_int64 sqlite3BtreeMaxRecordSize(BtCursor*);

int sqlite3BtreeIntegrityCheck(
  sqlite3 *db,  /* Database connection that is running the check */
  Btree *p,     /* The btree to be checked */
  Pgno *aRoot,  /* An array of root pages numbers for individual trees */
  sqlite3_value *aCnt,  /* OUT: entry counts for each btree in aRoot[] */
  int nRoot,    /* Number of entries in aRoot[] */
  int mxErr,    /* Stop reporting errors after this many */
  int *pnErr,   /* OUT: Write number of errors seen to this variable */
  char **pzOut  /* OUT: Write the error message string here */
);
struct Pager *sqlite3BtreePager(Btree*);
i64 sqlite3BtreeRowCountEst(BtCursor*);

#ifndef SQLITE_OMIT_INCRBLOB
int sqlite3BtreePayloadChecked(BtCursor*, u32 offset, u32 amt, void*);
int sqlite3BtreePutData(BtCursor*, u32 offset, u32 amt, void*);
void sqlite3BtreeIncrblobCursor(BtCursor *);
#endif
void sqlite3BtreeClearCursor(BtCursor *);
int sqlite3BtreeSetVersion(Btree *pBt, int iVersion);
int sqlite3BtreeCursorHasHint(BtCursor*, unsigned int mask);
int sqlite3BtreeIsReadonly(Btree *pBt);
int sqlite3HeaderSizeBtree(void);

#ifdef SQLITE_DEBUG
sqlite3_uint64 sqlite3BtreeSeekCount(Btree*);
#else
# define sqlite3BtreeSeekCount(X) 0
#endif

#ifndef NDEBUG
int sqlite3BtreeCursorIsValid(BtCursor*);
#endif
int sqlite3BtreeCursorIsValidNN(BtCursor*);

int sqlite3BtreeCount(sqlite3*, BtCursor*, i64*);

#ifdef SQLITE_TEST
int sqlite3BtreeCursorInfo(BtCursor*, int*, int);
void sqlite3BtreeCursorList(Btree*);
#endif

#ifndef SQLITE_OMIT_WAL
  int sqlite3BtreeCheckpoint(Btree*, int, int *, int *);
#endif

int sqlite3BtreeTransferRow(BtCursor*, BtCursor*, i64);

void sqlite3BtreeClearCache(Btree*);

/*
** If we are not using shared cache, then there is no need to
** use mutexes to access the BtShared structures.  So make the
** Enter and Leave procedures no-ops.
*/
#ifndef SQLITE_OMIT_SHARED_CACHE
  void sqlite3BtreeEnter(Btree*);
  void sqlite3BtreeEnterAll(sqlite3*);
  int sqlite3BtreeSharable(Btree*);
  void sqlite3BtreeEnterCursor(BtCursor*);
  int sqlite3BtreeConnectionCount(Btree*);
#else
# define sqlite3BtreeEnter(X) 
# define sqlite3BtreeEnterAll(X)
# define sqlite3BtreeSharable(X) 0
# define sqlite3BtreeEnterCursor(X)
# define sqlite3BtreeConnectionCount(X) 1
#endif

#if !defined(SQLITE_OMIT_SHARED_CACHE) && SQLITE_THREADSAFE
  void sqlite3BtreeLeave(Btree*);
  void sqlite3BtreeLeaveCursor(BtCursor*);
  void sqlite3BtreeLeaveAll(sqlite3*);
#ifndef NDEBUG
  /* These routines are used inside assert() statements only. */
  int sqlite3BtreeHoldsMutex(Btree*);
  int sqlite3BtreeHoldsAllMutexes(sqlite3*);
  int sqlite3SchemaMutexHeld(sqlite3*,int,Schema*);
#endif
#else

# define sqlite3BtreeLeave(X)
# define sqlite3BtreeLeaveCursor(X)
# define sqlite3BtreeLeaveAll(X)

# define sqlite3BtreeHoldsMutex(X) 1
# define sqlite3BtreeHoldsAllMutexes(X) 1
# define sqlite3SchemaMutexHeld(X,Y,Z) 1
#endif


#endif /* SQLITE_BTREE_H */
