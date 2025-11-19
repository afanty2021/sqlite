[根目录](../../CLAUDE.md) > **ext (扩展模块)**

# SQLite 扩展模块

## 模块职责

ext/ 目录包含 SQLite 的各种功能扩展，这些扩展可以单独编译为动态库，也可以编译到 SQLite 核心中。扩展提供了全文搜索、空间索引、JSON 处理、加密等高级功能。

## 入口与启动

### 扩展加载机制

```c
// 运行时加载扩展
sqlite3_load_extension(db, "fts5", NULL, NULL);

// 编译时包含扩展
#define SQLITE_ENABLE_FTS5
```

### 扩展入口点

每个扩展都实现了标准的 `sqlite3_extension_init()` 函数：

```c
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_extension_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
);
```

## 对外接口

### 核心扩展模块

| 扩展 | 路径 | 主要功能 | API 特点 |
|------|------|----------|----------|
| FTS3/4 | `fts3/` | 全文搜索引擎 | 虚拟表接口，BM25 排名 |
| FTS5 | `fts5/` | 新一代全文搜索 | 改进的索引和查询性能 |
| RTREE | `rtree/` | 空间索引 | R-Tree 算法，范围查询 |
| ICU | `icu/` | 国际化支持 | Unicode 处理，排序规则 |
| Session | `session/` | 数据变更跟踪 | 增量同步，冲突解决 |
| RBU | `rbu/` | 增量备份 | 只读备份，事务安全 |

### 杂项扩展 (misc/)

`misc/` 目录包含许多小型单文件扩展：

- **amatch.c** - 近似匹配
- **csv.c** - CSV 虚拟表
- **json1.c** - JSON 处理（现已成为核心功能）
- **spellfix.c** - 拼写纠错
- **series.c** - 数字序列生成
- **regexp.c** - 正则表达式支持
- **zipfile.c** - ZIP 归档支持

## 关键依赖与配置

### FTS5 依赖

```c
// 启用 FTS5
#define SQLITE_ENABLE_FTS5
#include "fts5.h"

// 创建 FTS5 表
CREATE VIRTUAL TABLE docs USING fts5(title, content);
```

### RTREE 配置

```c
// 启用 RTREE
#define SQLITE_ENABLE_RTREE

// 创建空间索引
CREATE VIRTUAL TABLE boundaries USING rtree(
  id, minX, maxX, minY, maxY
);
```

### 构建 FTS5 扩展

```bash
# 作为动态扩展构建
gcc -fPIC -shared fts5.c -o libfts5.so

# 作为内置功能构建
gcc -DSQLITE_ENABLE_FTS5 sqlite3.c -o sqlite3_with_fts5
```

## 数据模型

### FTS5 虚拟表结构

```c
// FTS5 内部表结构
CREATE TABLE '%_content'(rowid, docid, '%_cols');
CREATE TABLE '%_config'(k PRIMARY KEY, v);
CREATE TABLE '%_data'(id PRIMARY KEY, block);
CREATE TABLE '%_idx'(segid, term, pgno, PRIMARY KEY(segid, term));
CREATE TABLE '%_docsize'(docid PRIMARY KEY, sz);
```

### RTREE 空间索引

```c
// R-Tree 节点结构
typedef struct RtreeNode RtreeNode;
typedef struct RtreeCell RtreeCell;
struct RtreeCell {
  i64 iRowid;        // 行 ID
  RtreeCoord aCoord[2*RTREE_MAX_DIMENSIONS];  // 边界坐标
};
```

### Session 变更跟踪

```c
// 变更跟踪结构
typedef struct sqlite3_session sqlite3_session;
typedef struct sqlite3_changeset_iter sqlite3_changeset_iter;
```

## 测试与质量

### FTS5 测试

```bash
# FTS5 测试文件
cd ext/fts5/test
../testfixture ../fts5aa.test
```

### RTREE 测试

```bash
# RTREE 测试
cd ext/rtree
sqlite3 test.db < test_rtree.sql
```

### 质量检查

- **内存安全**：所有扩展都通过 OOM 测试
- **并发安全**：支持多线程环境
- **接口兼容**：遵循 SQLite 虚拟表接口标准

## 主要扩展详解

### FTS5 全文搜索引擎

**核心特性**：
- 基于 BM25 的相关性排名
- 支持前缀查询和 NEAR 查询
- 可分批索引大数据集
- 支持自定义分词器

**扩展 API 接口**：
```c
typedef struct Fts5ExtensionApi Fts5ExtensionApi;
typedef struct Fts5Context Fts5Context;
typedef struct Fts5PhraseIter Fts5PhraseIter;

// 扩展函数类型定义
typedef void (*fts5_extension_function)(
  const Fts5ExtensionApi *pApi,
  Fts5Context *pFts,
  sqlite3_context *pCtx,
  int nVal,
  sqlite3_value **apVal
);

// 核心 API 函数
int xColumnTotalSize(Fts5Context*, int iCol, sqlite3_int64 *pnToken);
int xColumnCount(Fts5Context*);
int xColumnSize(Fts5Context*, int iCol, int *pnToken);
int xPhraseCount(Fts5Context*);
int xPhraseSize(Fts5Context*, int iPhrase);
int xInstCount(Fts5Context*, int *pnInst);
```

**关键文件**：
- `fts5.h` - 公共接口定义
- `fts5.c` - 主要实现（需要生成）
- `fts5_config.c` - 配置管理
- `fts5_index.c` - 索引实现

### JSON 核心功能（从扩展移入）

**核心特性**：
- **JSONB 二进制格式**：SQLite 3.45.0+ 新增的二进制 JSON 表示
- **性能优化**：JSONB 比文本 JSON 快约 3 倍
- **空间效率**：JSONB 通常比文本 JSON 小 5-10%
- **兼容性**：完全支持 RFC-8259，并接受 JSON-5 扩展

**JSONB 编码格式**：
```c
/* JSONB 元素类型定义 */
#define JSONB_NULL     0   /* "null" */
#define JSONB_TRUE     1   /* "true" */
#define JSONB_FALSE    2   /* "false" */
#define JSONB_INT      3   /* 整数 */
#define JSONB_INT5     4   /* JSON5 整数格式 */
#define JSONB_FLOAT    5   /* 浮点数 */
#define JSONB_FLOAT5   6   /* JSON5 浮点数格式 */
#define JSONB_TEXT     7   /* 兼容 JSON 和 SQL 的文本 */
#define JSONB_TEXTJ    8   /* 包含 JSON 转义的文本 */
#define JSONB_TEXT5    9   /* 包含 JSON-5 转义的文本 */
#define JSONB_TEXTRAW 10   /* 需要 JSON 转义的 SQL 文本 */
#define JSONB_ARRAY   11   /* 数组 */
#define JSONB_OBJECT  12   /* 对象 */
```

**JSONB 头部结构**：
- **低 4 位**：元素类型 (0-12)
- **高 4 位**：头部和负载大小信息
- **变长头部**：1-9 字节，支持最大 2GB 负载

**关键文件**：
- `src/json.c` - JSON 和 JSONB 核心实现
- `doc/jsonb.md` - JSONB 格式详细文档

### RTREE 空间索引

**核心特性**：
- 基于 R-Tree 算法的空间索引
- 支持 1-5 维空间数据
- 包含函数：`Within`, `Contains`, `Intersects`
- 支持动态扩展和收缩

**API 接口**：
```c
// RTREE 初始化函数
int sqlite3RtreeInit(sqlite3 *db);

// 创建空间索引
CREATE VIRTUAL TABLE bounds USING rtree(
  id,              -- 主键
  minX, maxX,      -- X 坐标范围
  minY, maxY       -- Y 坐标范围
);

// 空间查询函数
SELECT * FROM bounds
WHERE minX>=?1 AND maxX<=?2 AND minY>=?3 AND maxY<=?4;
```

**关键文件**：
- `rtree.c` - 主要实现
- `rtree.h` - 公共接口定义
- `sqlite3rtree.h` - 外部 API 头文件

### Session 扩展

**核心特性**：
- 跟踪数据库的所有变更
- 生成变更集用于同步
- 支持冲突检测和解决
- 可增量应用到其他数据库

**变更跟踪流程**：
```c
// 创建会话
sqlite3_session *pSession;
sqlite3_session_create(db, "main", &pSession);

// 附加要跟踪的表
sqlite3_session_attach(pSession, NULL);  // NULL 表示所有表

// 生成变更集
int nChangeset;
void *pChangeset;
sqlite3_session_changeset(pSession, &nChangeset, &pChangeset);

// 应用变更集到目标数据库
sqlite3_changeset_apply(db2, nChangeset, pChangeset,
                       NULL, NULL, NULL);
```

**关键文件**：
- `session.c` - 核心实现
- `sqlite3session.h` - 公共接口

## 扩展开发指南

### 创建新扩展

1. **实现虚拟表接口**：

```c
static sqlite3_module myModule = {
  .iVersion = 1,
  .xCreate = myCreate,
  .xConnect = myConnect,
  .xBestIndex = myBestIndex,
  .xDisconnect = myDisconnect,
  .xDestroy = myDestroy,
  .xOpen = myOpen,
  .xClose = myClose,
  .xFilter = myFilter,
  .xNext = myNext,
  .xEof = myEof,
  .xColumn = myColumn,
  .xRowid = myRowid,
  // ... 其他回调
};
```

2. **注册扩展**：

```c
int sqlite3_extension_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  SQLITE_EXTENSION_INIT2(pApi);
  return sqlite3_create_module(db, "myextension", &myModule, 0);
}
```

### JSON 扩展函数开发

```c
// 自定义 JSON 函数示例
static void my_json_function(
  sqlite3_context *ctx,
  int argc,
  sqlite3_value **argv
){
  JsonParse x;  // JSON 解析结构
  const char *zJson;
  int nJson;

  zJson = (const char*)sqlite3_value_text(argv[0]);
  nJson = sqlite3_value_bytes(argv[0]);

  if( jsonParse(&x, ctx, zJson, nJson) ){
    // 解析成功，处理 JSON
    jsonReturn(&x, ctx, 0);
  }else{
    // 解析失败，设置错误
    sqlite3_result_error(ctx, x.zErr, -1);
  }

  jsonParseReset(&x);
}
```

### 调试技巧

- 使用 `PRAGMA vdbe_debug` 查看虚拟机操作
- 启用 `SQLITE_DEBUG` 宏获得额外检查
- 使用测试框架验证扩展功能

## 常见问题 (FAQ)

### Q: FTS5 和 FTS3/4 有什么区别？

A: FTS5 是 FTS3/4 的重写版本，具有更好的性能、更简单的 API 和更强的查询能力。

### Q: JSONB 相比文本 JSON 有什么优势？

A: JSONB 是二进制格式，解析速度快约 3 倍，存储空间节省 5-10%，同时保持完全的 API 兼容性。

### Q: 如何为自定义数据类型创建扩展？

A: 实现自定义函数和聚合函数，或创建虚拟表来处理特殊数据类型。

### Q: 扩展是否支持加密？

A: SQLite 支持通过 SEE（SQLite Encryption Extension）进行数据库加密，但这属于商业扩展。

## 相关文件清单

### 主要扩展目录
- `fts3/` - FTS3/4 全文搜索
- `fts5/` - FTS5 全文搜索
- `rtree/` - 空间索引
- `icu/` - 国际化支持
- `session/` - 变更跟踪
- `rbu/` - 增量备份

### 杂项扩展 (misc/)
- `amatch.c` - 近似匹配
- `csv.c` - CSV 虚拟表
- `spellfix.c` - 拼写纠错
- `series.c` - 数字序列
- `regexp.c` - 正则表达式
- `zipfile.c` - ZIP 支持
- `vsv.c` - 增强型 CSV

### 辅助工具
- `expert/` - 查询优化建议工具
- `lsm1/` - LSM 存储引擎实验
- `wasm/` - WebAssembly 支持

### JSON 相关（核心功能）
- `src/json.c` - JSON 和 JSONB 核心实现
- `doc/jsonb.md` - JSONB 格式文档

## 变更记录 (Changelog)

### 2025-11-19 15:45:30 - 第2轮深度补捞完成
- **深度分析 FTS5 扩展 API**：详细分析 `fts5.h` 接口定义，包含扩展函数类型和核心 API
- **挖掘 JSONB 技术细节**：深入分析 `src/json.c` 中的 JSONB 二进制格式实现
- **RTREE 接口解析**：明确 RTREE 的 API 结构和使用方式
- **Session 扩展流程梳理**：详细说明变更跟踪的完整流程
- **技术实现细节**：JSONB 编码格式、FTS5 扩展接口等核心算法细节
- **代码示例补充**：增加实际的扩展开发和 JSON 函数开发示例
- **覆盖状态**：阶段C深度补捞完成
- **扩展覆盖**：主要扩展的核心技术细节已全面分析

### 2025-11-19 09:20:15 - 扩展模块文档创建
- 建立扩展模块文档结构
- 分析主要扩展：FTS5、RTREE、Session、RBU
- 识别 30+ 杂项扩展
- **覆盖状态**：阶段B扫描完成
- **扩展覆盖**：5个主要扩展 + misc/ 目录
- **缺口分析**：部分小扩展的详细信息待补充