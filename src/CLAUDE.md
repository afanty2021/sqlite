[根目录](../../CLAUDE.md) > **src (核心引擎)**

# SQLite 核心引擎模块

## 模块职责

src/ 目录包含 SQLite 数据库引擎的完整实现，包括 SQL 解析、查询优化、虚拟机执行、存储管理等核心功能。这是 SQLite 的心脏部分，所有数据库操作都在这里实现。

## 入口与启动

### 核心入口文件

- **main.c** - 数据库连接管理和主要 API 实现
- **sqlite.h.in** - 公共 API 接口定义模板
- **sqliteInt.h** - 内部数据结构和接口定义

### 初始化流程

1. `sqlite3_initialize()` - 全局初始化
2. `sqlite3_open()` - 创建数据库连接
3. `sqlite3_prepare()` - 编译 SQL 语句
4. `sqlite3_step()` - 执行查询
5. `sqlite3_finalize()` - 释放语句

## 对外接口

### 公共 API (来自 sqlite.h.in)

- **连接管理**：`sqlite3_open()`, `sqlite3_close()`, `sqlite3_config()`
- **语句执行**：`sqlite3_prepare()`, `sqlite3_step()`, `sqlite3_finalize()`
- **参数绑定**：`sqlite3_bind_*()` 系列函数
- **结果获取**：`sqlite3_column_*()` 系列函数
- **事务控制**：`sqlite3_exec()`, `sqlite3_commit_hook()`
- **扩展接口**：`sqlite3_create_function()`, `sqlite3_load_extension()`

### 内部模块接口

- **B-Tree 接口**：`btree.h` - 存储引擎抽象层
- **页面缓存**：`pcache.h` - 页面缓存管理
- **事务管理**：`pager.h` - 事务和日志管理
- **虚拟机**：`vdbe.h` - 虚拟机接口

## 关键依赖与配置

### 核心组件依赖

```c
// 核心内部结构
#include "sqliteInt.h"     // 内部定义
#include "btree.h"         // B-Tree 接口
#include "vdbe.h"          // 虚拟机接口
#include "pager.h"         // 事务管理
#include "os.h"            // 操作系统抽象
```

### 编译时配置

- **功能特性**：通过 `#ifdef SQLITE_*` 控制功能开关
- **内存配置**：`SQLITE_CONFIG_MALLOC`, `SQLITE_CONFIG_PAGECACHE`
- **线程模式**：`SQLITE_CONFIG_SINGLETHREAD`, `SQLITE_CONFIG_MULTITHREAD`

## 数据模型

### 核心数据结构

```c
// 数据库连接
struct sqlite3 {
    Vdbe *pVdbe;           // 准备好的语句列表
    Btree *aDb[2];         // 数据库文件连接
    CollSeq *pCollSeq;     // 排序序列
    FuncDefHash *pFunc;    // 函数哈希表
    // ... 更多字段
};

// B-Tree 游标
struct BtCursor {
    Btree *pBtree;         // 关联的 B-Tree
    Pgno pgnoRoot;         // 根页面号
    CellInfo info;         // 单元格信息
    // ... 更多字段
};

// 虚拟机实例
struct Vdbe {
    sqlite3 *db;           // 数据库连接
    VdbeFrame *pFrame;     // 执行帧
    Mem *aMem;             // 内存寄存器
    Op *aOp;               // 操作码数组
    // ... 更多字段
};
```

### 页面结构

- **B-Tree 页面**：存储表数据、索引数据
- **溢出页面**：存储大数据的溢出部分
- **空闲页面**：页面回收机制
- **锁页面**：并发控制信息

## 核心引擎深度分析

### VDBE 虚拟机实现

**核心文件**：`vdbe.c` - 虚拟机主执行引擎

**虚拟机架构**：
```c
// 虚拟机执行主循环
int sqlite3VdbeExec(Vdbe *p){
  Mem *pIn1 = 0;           // 输入寄存器 1
  Mem *pIn2 = 0;           // 输入寄存器 2
  Mem *pIn3 = 0;           // 输入寄存器 3
  Mem *pOut = 0;           // 输出寄存器

  // 主执行循环
  while(1){
    // 获取当前操作码
    pOp = &p->aOp[pc];

    // 根据操作码分发执行
    switch( pOp->opcode ){
      // 各种 VDBE 指令实现
    }
  }
}
```

**调试和测试功能**：
```c
// 全局测试变量
#ifdef SQLITE_TEST
int sqlite3_search_count = 0;      // 搜索计数
int sqlite3_interrupt_count = 0;   // 中断计数
int sqlite3_sort_count = 0;        // 排序计数
int sqlite3_max_blobsize = 0;      // 最大 Blob 大小
int sqlite3_found_count = 0;       // 查找计数
#endif

// 内存调试宏
#ifdef SQLITE_DEBUG
static void test_trace_breakpoint(int pc, Op *pOp, Vdbe *v);
#define memAboutToChange(P,M) sqlite3VdbeMemAboutToChange(P,M)
#endif
```

**分支覆盖率跟踪**：
```c
// VDBE 分支覆盖率跟踪
#if !defined(SQLITE_VDBE_COVERAGE)
# define VdbeBranchTaken(I,M)
#else
# define VdbeBranchTaken(I,M) vdbeTakeBranch(pOp->iSrcLine,I,M)
static void vdbeTakeBranch(u32 iSrcLine, u8 I, u8 M);
#endif
```

**游标分配和管理**：
```c
// VDBE 游标分配
static VdbeCursor *allocateCursor(
  Vdbe *p,              // 虚拟机实例
  int iCur,             // 游标编号
  int nField,           // 字段数量
  u8 eCurType           // 游标类型
){
  Mem *pMem = iCur>0 ? &p->aMem[p->nMem-iCur] : p->aMem;
  i64 nByte = SZ_VDBECURSOR(nField);

  if( eCurType==CURTYPE_BTREE )
    nByte += sqlite3BtreeCursorSize();

  // 分配和初始化游标
  p->apCsr[iCur] = pCx = (VdbeCursor*)pMem->zMalloc;
  memset(pCx, 0, offsetof(VdbeCursor,pAltCursor));
  pCx->eCurType = eCurType;
  pCx->nField = nField;
  return pCx;
}
```

**数值类型转换和亲和性**：
```c
// 应用数值亲和性转换
static void applyNumericAffinity(Mem *pRec, int bTryForInt){
  double rValue;
  u8 enc = pRec->enc;
  int rc;

  assert( (pRec->flags & (MEM_Str|MEM_Int|MEM_Real|MEM_IntReal))==MEM_Str );
  rc = sqlite3AtoF(pRec->z, &rValue, pRec->n, enc);
  if( rc<=0 ) return;

  if( rc==1 && alsoAnInt(pRec, rValue, &pRec->u.i) ){
    pRec->flags |= MEM_Int;
  }else{
    pRec->u.r = rValue;
    pRec->flags |= MEM_Real;
    if( bTryForInt ) sqlite3VdbeIntegerAffinity(pRec);
  }

  // 清除字符串表示，因为 TEXT->NUMERIC 是多对一转换
  pRec->flags &= ~MEM_Str;
}

// 应用通用亲和性
static void applyAffinity(Mem *pRec, u8 affinity, int encoding){
  switch( affinity ){
    case SQLITE_AFF_TEXT:    // 转换为文本
    case SQLITE_AFF_NONE:    // 无变化
    case SQLITE_AFF_BLOB:    // 无变化
      break;

    case SQLITE_AFF_NUMERIC: // 优先转换为数值
    case SQLITE_AFF_INTEGER: // 优先转换为整数
    case SQLITE_AFF_REAL:    // 转换为实数
      if( (pRec->flags & MEM_Str)==0 ) return;
      applyNumericAffinity(pRec, affinity==SQLITE_AFF_INTEGER);
      break;
  }
}
```

### B-Tree 存储引擎

**核心文件**：`btree.c` - B-Tree 存储引擎实现

**数据库文件头标识**：
```c
// SQLite 数据库文件头魔术字符串
static const char zMagicHeader[] = SQLITE_FILE_HEADER;
```

**共享缓存支持**：
```c
// 共享缓存全局链表
#ifdef SQLITE_TEST
BtShared *SQLITE_WSD sqlite3SharedCacheList = 0;
#else
static BtShared *SQLITE_WSD sqlite3SharedCacheList = 0;
#endif

// 启用/禁用共享缓存
int sqlite3_enable_shared_cache(int enable){
  sqlite3GlobalConfig.sharedCacheEnabled = enable;
  return SQLITE_OK;
}
```

**页面损坏处理**：
```c
// 页面损坏错误处理
#ifdef SQLITE_DEBUG
int corruptPageError(int lineno, MemPage *p){
  char *zMsg;
  sqlite3BeginBenignMalloc();
  zMsg = sqlite3_mprintf("database corruption page %u of %s",
             p->pgno, sqlite3PagerFilename(p->pBt->pPager, 0)
  );
  sqlite3EndBenignMalloc();
  if( zMsg ){
    sqlite3ReportError(SQLITE_CORRUPT, lineno, zMsg);
  }
  sqlite3_free(zMsg);
  return SQLITE_CORRUPT_BKPT;
}
# define SQLITE_CORRUPT_PAGE(pMemPage) corruptPageError(__LINE__, pMemPage)
#else
# define SQLITE_CORRUPT_PAGE(pMemPage) SQLITE_CORRUPT_PGNO(pMemPage->pgno)
#endif
```

**锁定跟踪调试**：
```c
// 共享锁跟踪（调试用）
#if 0
static void sharedLockTrace(
  BtShared *pBt,
  const char *zMsg,
  int iRoot,
  int eLockType
){
  BtLock *pLock;
  if( iRoot>0 ){
    printf("%s-%p %u%s:", zMsg, pBt, iRoot, eLockType==READ_LOCK?"R":"W");
  }else{
    printf("%s-%p:", zMsg, pBt);
  }

  for(pLock=pBt->pLock; pLock; pLock=pLock->pNext){
    printf(" %p/%u%s", pLock->pBtree, pLock->iTable,
           pLock->eLock==READ_LOCK ? "R" : "W");
  }
  printf("\n");
  fflush(stdout);
}
#endif
```

### 页面管理器 (Pager)

**核心文件**：`pager.c` - 事务和页面管理

**Pager 状态机**：
```c
/*
** Pager 状态转换图：
**
**   OPEN <------+------+
**     |         |      |
**     V         |      |
**   READER-------+      |
**     |              |
**     V              |
**   WRITER_LOCKED-----> ERROR
**     |              ^
**     V              |
**   WRITER_CACHEMOD--->|
**     |              |
**     V              |
**   WRITER_DBMOD----->|
**     |              |
**     V              |
**   WRITER_FINISHED-->+
**
** 状态转换和执行函数：
**   OPEN -> READER           [sqlite3PagerSharedLock]
**   READER -> OPEN           [pager_unlock]
**   READER -> WRITER_LOCKED  [sqlite3PagerBegin]
**   WRITER_LOCKED -> WRITER_CACHEMOD [pager_open_journal]
**   WRITER_CACHEMOD -> WRITER_DBMOD [syncJournal]
**   WRITER_DBMOD -> WRITER_FINISHED [sqlite3PagerCommitPhaseOne]
**   WRITER_*** -> READER      [pager_end_transaction]
**   WRITER_*** -> ERROR       [pager_error]
**   ERROR -> OPEN            [pager_unlock]
*/
```

**Pager 状态说明**：

1. **OPEN**：启动状态，锁状态未知，数据库大小不可信
2. **READER**：读取状态，持有共享锁，数据库大小已知
3. **WRITER_LOCKED**：写者锁定状态，已获得保留锁，但未打开日志
4. **WRITER_CACHEMOD**：缓存修改状态，日志已打开，但未同步到磁盘
5. **WRITER_DBMOD**：数据库修改状态，已同步日志，准备提交
6. **WRITER_FINISHED**：写完成状态，准备结束事务
7. **ERROR**：错误状态，需要清理和重置

**日志和事务不变性**：
```c
/*
** 回滚日志模式的不变性：
**
** (1) 数据库文件页面只有在以下条件满足时才被覆盖：
**     (a) 页面和同扇区的其他页面都是可覆盖的
**     (b) 原子页面写优化启用，且事务只修改单页
**
** (2) 写入回滚日志的页面内容必须与以下内容完全匹配：
**     (a) 写入日志时的数据库文件内容
**     (b) 事务开始时的数据库内容
**
** (3) 数据库文件的写入必须是页面大小的整数倍且页面对齐
**
** (4) 数据库文件的读取必须是页面对齐的，或者读取文件前100字节
**
** (5) 在删除、截断或清零回滚日志之前，所有数据库写入必须同步
**
** ... 更多不变性规则
*/
```

**调试和跟踪**：
```c
// Pager 调试宏
#if 0
int sqlite3PagerTrace=1;  // 启用跟踪
#define sqlite3DebugPrintf printf
#define PAGERTRACE(X)     if( sqlite3PagerTrace ){ sqlite3DebugPrintf X; }
#else
#define PAGERTRACE(X)
#endif

// 文件描述符跟踪宏
#define PAGERID(p) (SQLITE_PTR_TO_INT(p->fd))
#define FILEHANDLEID(fd) (SQLITE_PTR_TO_INT(fd))
```

### SQL 解析器

**核心文件**：`parse.y` - Lemon LALR(1) SQL 语法

**解析器配置**：
```yacc
// Lemon 解析器配置
%include {
  // 禁用错误恢复
  #define YYNOERRORRECOVERY 1

  // 使用 testcase() 宏
  #define yytestcase(X) testcase(X)

  // 解析器永远不会被空指针调用
  #define YYPARSEFREENEVERNULL 1

  // Amalgamation 构建优化
  #ifdef SQLITE_AMALGAMATION
  # define sqlite3Parser_ENGINEALWAYSONSTACK 1
  #endif

  // 内存分配参数类型
  #define YYMALLOCARGTYPE  u64
}
```

**触发器事件结构**：
```c
// 触发器事件描述
struct TrigEvent {
  int a;        // 事件类型 (TK_UPDATE, TK_INSERT, TK_DELETE, TK_INSTEAD)
  IdList * b;   // 更新的列列表 (如 UPDATE ON (a,b,c))
};
```

**窗口框架边界**：
```c
// 窗口函数框架边界
struct FrameBound {
  int eType;    // 边界类型
  Expr *pExpr;  // 边界表达式
};
```

**解析器辅助函数**：
```c
// 生成语法错误
static void parserSyntaxError(Parse *pParse, Token *p){
  sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", p);
}

// 禁用 lookaside 内存分配
static void disableLookaside(Parse *pParse){
  sqlite3 *db = pParse->db;
  pParse->disableLookaside++;
  #ifdef SQLITE_DEBUG
  pParse->isCreate = 1;
  #endif
  memset(&pParse->u1.cr, 0, sizeof(pParse->u1.cr));
  DisableLookaside;
}
```

**事务语法支持**：
```yacc
// 事务控制语法
cmd ::= BEGIN transtype(Y) trans_opt.
  {sqlite3BeginTransaction(pParse, Y);}

transtype(A) ::= .             {A = TK_DEFERRED;}
transtype(A) ::= DEFERRED(X).  {A = @X;}
transtype(A) ::= IMMEDIATE(X). {A = @X;}
transtype(A) ::= EXCLUSIVE(X). {A = @X;}

cmd ::= COMMIT|END(X) trans_opt.
  {sqlite3EndTransaction(pParse,@X);}

cmd ::= ROLLBACK(X) trans_opt.
  {sqlite3EndTransaction(pParse,@X);}
```

### 查询优化器

**核心文件**：`where.c` - WHERE 子句分析和优化

**扩展索引信息**：
```c
// 虚拟表扩展索引信息
typedef struct HiddenIndexInfo HiddenIndexInfo;
struct HiddenIndexInfo {
  WhereClause *pWC;        // WHERE 子句分析
  Parse *pParse;           // 解析上下文
  int eDistinct;           // sqlite3_vtab_distinct() 返回值
  u32 mIn;                 // <col> IN (...) 项的掩码
  u32 mHandleIn;           // vtab 处理的 <col> IN (...) 项
  sqlite3_value *aRhs[FLEXARRAY];  // 约束的 RHS 值
};
```

**优化器接口函数**：
```c
// 返回 WHERE 子句估计输出行数
LogEst sqlite3WhereOutputRowCount(WhereInfo *pWInfo);

// 返回 DISTINCT 处理方式
int sqlite3WhereIsDistinct(WhereInfo *pWInfo);

// 返回 ORDER BY 满足度
int sqlite3WhereIsOrdered(WhereInfo *pWInfo);

// ORDER BY LIMIT 优化标签
int sqlite3WhereOrderByLimitOptLabel(WhereInfo *pWInfo);

// Min/Max 优化提前退出
void sqlite3WhereMinMaxOptEarlyOut(Vdbe *v, WhereInfo *pWInfo);

// ONEPASS 优化检查
int sqlite3WhereOkOnePass(WhereInfo *pWInfo, int *aiCur);

// 延迟查找检查
int sqlite3WhereUsesDeferredSeek(WhereInfo *pWInfo);
```

**ORDER BY LIMIT 优化**：
```c
/*
** ORDER BY LIMIT 优化：
** 如果最内层循环按递增顺序发出行，且最后发出的行不符合排序器，
** 则可以跳过当前内层迭代的所有后续行（因为它们也不会符合排序器）。
**
** 这种情况下，跳转到第二内层循环的继续部分。
** 如果 ORDER BY LIMIT 优化不适用，则跳转到最内层循环的继续部分。
**
** 返回最内层循环总是安全的，但返回第二内层循环是一个性能优化。
*/
int sqlite3WhereOrderByLimitOptLabel(WhereInfo *pWInfo){
  WhereLevel *pInner;
  if( !pWInfo->bOrderedInnerLoop ){
    // ORDER BY LIMIT 优化不适用
    return pWInfo->iContinue;
  }
  pInner = &pWInfo->a[pWInfo->nLevel-1];
  assert( pInner->addrNxt!=0 );
  return pInner->pRJ ? pWInfo->iContinue : pInner->addrNxt;
}
```

**Min/Max 优化提前退出**：
```c
/*
** Min/Max 优化提前退出：
** 在处理 min/max 聚合步骤后，检查是否需要额外循环。
** 如果输出顺序确保已经找到正确答案，则跳过后续处理。
**
** 任何额外的 OP_Goto 都是优化，不影响最终结果，只是让答案更快出现。
*/
void sqlite3WhereMinMaxOptEarlyOut(Vdbe *v, WhereInfo *pWInfo){
  WhereLevel *pInner;
  int i;
  if( !pWInfo->bOrderedInnerLoop ) return;
  if( pWInfo->nOBSat==0 ) return;

  for(i=pWInfo->nLevel-1; i>=0; i--){
    pInner = &pWInfo->a[i];
    if( (pInner->pWLoop->wsFlags & WHERE_COLUMN_IN)!=0 ){
      sqlite3VdbeGoto(v, pInner->addrNxt);
      return;
    }
  }
  sqlite3VdbeGoto(v, pWInfo->iBreak);
}
```

## 测试与质量

### 测试文件模式

- **test*.c** - 单元测试和集成测试
- **主测试程序**：`test1.c` → `test9.c`, `test_autoext.c` 等
- **专用测试**：`fkey.c`, `memdb.c` 等功能特定测试

### 构建测试

```bash
make testfixture          # 构建测试框架
./testfixture test/main.test  # 运行主测试套件
```

### 调试工具

- **内存调试**：`SQLITE_MEMDEBUG` 宏
- **虚拟机调试**：`.explain` 命令
- **B-Tree 检查**：`PRAGMA integrity_check`

## 关键源文件说明

### 解析器和语法分析

- **parse.y** - SQL 语法的 LALR(1) 定义
- **parse.c** - Lemon 生成的解析器代码
- **resolve.c** - 名称解析和语义分析

### 查询优化器

- **where.c** - WHERE 子句分析和优化
- **select.c** - SELECT 语句处理
- **expr.c** - 表达式求值

### 虚拟机执行

- **vdbe.c** - 虚拟机主循环实现
- **vdbeaux.c** - 虚拟机辅助函数
- **vdbeapi.c** - 虚拟机 API 实现

### 存储引擎

- **btree.c** - B-Tree 存储引擎
- **pager.c** - 页面管理和事务控制
- **os_unix.c** / **os_win.c** - 操作系统接口

### 工具和辅助

- **func.c** - 内置 SQL 函数
- **date.c** - 日期时间处理
- **json.c** - JSON 扩展功能
- **analyze.c** - 统计信息

## 常见问题 (FAQ)

### Q: 如何添加新的 SQL 函数？

A: 在 `func.c` 中实现 `sqlite3_create_function()` 接口，参考现有函数的实现模式。

### Q: 虚拟机指令如何工作？

A: VDBE 指令在 `vdbe.c` 的 `sqlite3VdbeExec()` 函数中执行，每个指令有特定的操作码和操作数。

### Q: B-Tree 的页面大小如何影响性能？

A: 页面大小通过 `PRAGMA page_size` 设置，影响 I/O 效率和内存使用。常见大小为 4096 字节。

### Q: Pager 的事务模型如何保证 ACID 特性？

A: 通过回滚日志、WAL 模式和严格的同步机制保证原子性、一致性、隔离性和持久性。

### Q: WHERE 子句优化器使用哪些策略？

A: 包括索引选择、查询计划优化、ORDER BY 优化、Min/Max 优化、LIMIT 优化等。

## 相关文件清单

### 核心实现文件
- `main.c` - 主要 API 实现
- `sqliteInt.h` - 内部定义
- `btree.c/.h` - B-Tree 实现
- `vdbe.c/.h` - 虚拟机实现
- `pager.c/.h` - 事务管理
- `os.c/.h` - 操作系统抽象

### 解析和优化
- `parse.y` - SQL 语法
- `where.c` - 查询优化
- `select.c` - SELECT 处理
- `expr.c` - 表达式处理

### 功能模块
- `func.c` - SQL 函数
- `date.c` - 日期时间
- `json.c` - JSON 支持
- `analyze.c` - 统计信息

### 测试文件
- `test1.c` ... `test9.c` - 主测试套件
- `fkey.c` - 外键测试
- `memdb.c` - 内存数据库测试

## 变更记录 (Changelog)

### 2025-11-19 15:45:30 - 第2轮深度补捞完成
- **VDBE 虚拟机深度解析**：详细分析 `vdbe.c` 执行引擎，包含主循环、调试支持、游标管理
- **数值类型亲和性机制**：深入分析 `applyNumericAffinity()` 和类型转换策略
- **B-Tree 存储引擎剖析**：分析 `btree.c` 中的共享缓存、页面损坏处理、锁定机制
- **Pager 事务管理详解**：详细说明 Pager 状态机、事务不变性、日志管理机制
- **SQL 解析器架构**：深入分析 `parse.y` 的 Lemon 语法、触发器事件、事务语法
- **查询优化器策略**：挖掘 `where.c` 中的优化算法，包括 ORDER BY LIMIT、Min/Max 优化
- **技术实现细节**：虚拟机指令分发、内存管理、分支覆盖率跟踪等核心算法
- **代码示例补充**：增加实际的核心组件使用示例和最佳实践
- **覆盖状态**：阶段C深度补捞完成
- **核心文件覆盖**：VDBE、B-Tree、Pager、解析器、优化器核心实现已全面分析

### 2025-11-19 09:20:15 - 核心模块文档创建
- 建立 src/ 模块文档结构
- 分析核心组件和数据结构
- 列出关键源文件和接口
- **覆盖状态**：阶段B扫描完成
- **文件覆盖**：约 150+ 核心文件已识别
- **缺口分析**：扩展模块和工具模块需进一步分析