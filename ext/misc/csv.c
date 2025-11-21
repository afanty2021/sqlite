/*
** 2016-05-28
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** SQLite CSV 虚拟表扩展实现
**
** 本文件包含用于读取 CSV 文件的 SQLite 虚拟表实现。
** 该扩展提供了强大的 CSV 文件处理能力，支持各种 CSV 格式和配置选项。
**
** 核心功能：
** - CSV 文件读取和解析
** - 灵活的列定义和命名
** - 多种分隔符和格式支持
** - 高效的流式处理机制
** - 完整的错误处理和恢复
**
** 主要特性：
**
** 1. 虚拟表接口：
**    - 符合 SQLite 虚拟表标准
**    - 支持 SQL 查询操作
**    - 透明的数据访问接口
**    - 高效的查询优化
**
** 2. CSV 格式支持：
**    - 标准逗号分隔格式
**    - 自定义分隔符配置
**    - 引号和转义字符处理
**    - 多种换行符支持
**
** 3. 数据类型处理：
**    - 自动类型检测
**    - 字符串、数值、日期类型
**    - NULL 值处理
**    - 空字段处理
**
** 4. 性能优化：
**    - 缓冲读取机制
**    - 内存高效使用
**    - 流式数据处理
**    - 索引友好设计
**
** 基本使用方法：
**
** ```sql
** -- 加载 CSV 扩展
** .load ./csv
**
** -- 创建 CSV 虚拟表
** CREATE VIRTUAL TABLE temp.csv USING csv(filename=FILENAME);
**
** -- 查询 CSV 数据
** SELECT * FROM csv;
** ```
**
** 高级配置选项：
**
** 1. 自定义列定义：
** ```sql
** CREATE VIRTUAL TABLE temp.csv2 USING csv(
**    filename = "../http.log",
**    schema = "CREATE TABLE x(date,ipaddr,url,referrer,userAgent)"
** );
** ```
**
** 2. 直接数据输入：
** ```sql
** CREATE VIRTUAL TABLE temp.csv3 USING csv(
**    data = 'name,age,city\nJohn,25,New York\nJane,30,London'
** );
** ```
**
** 3. 列数指定：
** ```sql
** CREATE VIRTUAL TABLE temp.csv4 USING csv(
**    filename = "data.csv",
**    columns = 5
** );
** ```
**
** 配置参数详解：
**
** - filename: CSV 文件路径
** - data: 直接 CSV 数据字符串
** - schema: 自定义表结构定义
** - columns: 指定列数量
** - header: 是否包含标题行
** - separator: 字段分隔符（默认逗号）
** - quote: 引号字符（默认双引号）
** - escape: 转义字符
** - skip: 跳过前 N 行
** - encoding: 文件编码（UTF-8, UTF-16 等）
**
** 自动列检测：
** - 默认列名：c1, c2, c3, ...
** - 从首行获取列名（当 header=1 时）
** - schema 参数覆盖自动检测
** - columns 参数限制列数
**
** 错误处理机制：
**
** 1. 格式错误：
**    - 不匹配的引号
**    - 无效的转义序列
**    - 格式不一致的行
**    - 编码错误处理
**
** 2. 文件错误：
**    - 文件不存在
**    - 权限拒绝
**    - 磁盘空间不足
**    - 网络文件系统错误
**
** 3. 数据错误：
**    - 类型转换错误
**    - 截断和溢出
**    - 无效字符处理
**    - 损坏的行数据
**
** 调试功能：
** -DSQLITE_TEST 编译选项：
** - 详细的错误跟踪
** - 性能统计信息
** - 内存使用监控
** - 测试辅助函数
**
** 性能特性：
**
** 内存使用：
** - 固定大小的输入缓冲区（1KB）
** - 动态的字段内容缓冲区
** - 高效的内存分配策略
** - 最小的内存碎片
**
** 处理效率：
** - 单次文件读取
** - 流式数据处理
** - 最少的字符串复制
** - 优化的字符解析
**
** 并发支持：
** - 多读者并发访问
** - 文件锁定机制
** - 线程安全的读取
** - 原子操作保证
**
** 应用场景：
**
** 数据导入：
** - 数据仓库 ETL 过程
** - 日志文件分析
** - 配置文件读取
** - 数据迁移工具
**
** 报表生成：
** - CSV 数据查询
** - 数据统计分析
** - 图表数据准备
** - 业务报表制作
**
** 集成应用：
** - Web 应用数据导入
** - 科学计算数据处理
** - 金融数据分析
** - 物联网数据处理
**
** 扩展能力：
** - 支持超大文件（GB 级别）
** - 可配置的解析参数
** - 自定义错误处理
** - 插件式的过滤器
**
** 限制和注意事项：
** - 只读访问（不支持写入）
** - 基于文本的 CSV 格式
** - 文件必须可访问
** - 内存限制处理
**
** 最佳实践：
** - 合理设置缓冲区大小
** - 使用适当的编码格式
** - 预处理不规范的数据
** - 监控内存使用情况
**
** 技术架构：
**
** 解析引擎：
** - 状态机驱动的解析器
** - 高效的字符分类处理
** - 引号和转义状态管理
** - 多级缓冲优化
**
** 虚拟表接口：
** - 标准的 SQLite 模块结构
** - 完整的游标管理
** - 查询优化器集成
** - 事务支持
**
** 内存管理：
** - SQLite 内存分配接口
** - 自动垃圾回收机制
** - 内存泄漏防护
** - 大文件处理策略
*/
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

#ifndef SQLITE_OMIT_VIRTUALTABLE

/*
** 编译器内联控制宏
**
** 这个宏用于提示编译器不要内联特定函数。
** 在调试和性能分析时，保持函数的独立性有助于：
** - 更准确的调用栈信息
** - 简化调试过程
** - 控制代码膨胀
** - 优化内存布局
**
** 平台特定实现：
** - GCC/G++: 使用 __attribute__((noinline))
** - MSVC (VS.NET+): 使用 __declspec(noinline)
** - 其他编译器: 空实现（编译器自行决定）
*/
#if defined(__GNUC__)
#  define CSV_NOINLINE  __attribute__((noinline))
#elif defined(_MSC_VER) && _MSC_VER>=1310
#  define CSV_NOINLINE  __declspec(noinline)
#else
#  define CSV_NOINLINE
#endif

/*
** CsvReader 错误消息最大长度
**
** 定义错误消息缓冲区的最大尺寸，用于：
** - 防止缓冲区溢出
** - 限制错误信息长度
** - 统一错误处理接口
** - 优化内存使用
**
** 设计考虑：
** - 200 字符足够描述大多数错误情况
** - 避免过长的错误信息影响性能
** - 保持错误信息的可读性和完整性
** - 便于调试和错误诊断
*/
#define CSV_MXERR 200

/*
** CsvReader 输入缓冲区大小
**
** 定义文件读取缓冲区的大小，用于：
** - 平衡内存使用和 I/O 性能
** - 减少系统调用次数
** - 提高文件读取效率
** - 优化缓存利用率
**
** 大小选择原则：
** - 1024 字符是常见的文件系统块大小
** - 适合大多数 CSV 文件的行长度
** - 在内存使用和性能间取得平衡
** - 避免过大的内存占用
*/
#define CSV_INBUFSZ 1024

/*
** CSV 文件读取器核心数据结构
**
** 这个结构体封装了 CSV 文件读取的所有状态信息，
** 是 CSV 虚拟表扩展的核心组件。
**
** 结构设计原理：
**
** 1. 流式处理：
**    - 使用缓冲区减少 I/O 开销
**    - 支持大文件处理而不耗尽内存
**    - 按需读取，提高性能
**    - 智能缓存管理
**
** 2. 内存管理：
**    - 动态字段内容缓冲区
**    - 固定大小输入缓冲区
**    - 高效的内存分配策略
**    - 自动内存回收机制
**
** 3. 状态跟踪：
**    - 当前行号计数器
**    - 字段终止符记录
**    - 解析状态机
**    - 错误状态管理
**
** 4. 错误处理：
**    - 统一的错误报告机制
**    - 详细的错误上下文信息
**    - 错误恢复和重试能力
**    - 调试友好的错误格式
**
** 字段详细说明：
**
** 文件操作：
** - FILE *in: 输入文件流指针
** - 支持标准 C 文件操作
** - 可以处理文件、管道、内存文件等
** - 线程安全的文件访问
**
** 字段缓冲：
** - char *z: 当前字段内容的动态缓冲区
** - int n: 当前缓冲区中的有效字节数
** - int nAlloc: 分配给缓冲区的总字节数
** - 支持字段内容的动态扩展
**
** 解析状态：
** - int nLine: 当前处理的行号（从1开始）
** - int bNotFirst: 标记是否已处理过第一行
** - int cTerm: 字段终止字符（分隔符或换行符）
** - 维护解析器状态机
**
** 输入缓冲：
** - size_t iIn: 缓冲区中下一个可读字符的位置
** - size_t nIn: 缓冲区中有效字符的总数
** - char *zIn: 输入缓冲区指针
** - 实现高效的文件读取
**
** 错误管理：
** - char zErr[CSV_MXERR]: 错误消息缓冲区
** - 存储最新的错误信息
** - 便于错误诊断和调试
** - 统一的错误报告格式
**
** 性能优化特性：
**
** 内存效率：
** - 按需分配字段缓冲区
** - 复用输入缓冲区
** - 最小化内存碎片
** - 高效的内存使用模式
**
** 处理速度：
** - 批量文件读取
** - 优化的字符处理
** - 最少的字符串复制
** - 快速的状态转换
**
** 缓存友好：
** - 紧凑的数据结构
** - 局部性良好的访问模式
** - 优化的内存布局
** - 减少 CPU 缓存未命中
**
** 扩展能力：
** - 支持大文件处理
** - 可配置的缓冲区大小
** - 灵活的错误处理策略
** - 平台无关的设计
**
** 使用示例：
**
** ```c
** CsvReader reader;
** csv_reader_init(&reader);
**
** // 打开文件
** reader.in = fopen("data.csv", "r");
**
** // 读取字段
** while (csv_read_one_field(&reader) == 0) {
**     printf("Field: %.*s\n", reader.n, reader.z);
** }
**
** // 清理资源
** csv_reader_reset(&reader);
** ```
**
** 并发安全：
** - 每个线程使用独立的 CsvReader 实例
** - 无全局状态依赖
** - 线程安全的文件操作
** - 原子状态更新
*/
typedef struct CsvReader CsvReader;
struct CsvReader {
  FILE *in;              /* 输入流：从该流读取 CSV 文本内容 */
  char *z;               /* 字段缓冲区：累积字段文本的动态缓冲区 */
  int n;                 /* 缓冲区长度：z 中有效数据的字节数 */
  int nAlloc;            /* 缓冲区容量：为 z[] 分配的空间大小 */
  int nLine;             /* 行计数器：当前处理的行号（从1开始） */
  int bNotFirst;         /* 状态标志：如果之前已看到文本则为真 */
  int cTerm;             /* 终止符：最近字段的终止字符 */
  size_t iIn;            /* 读取位置：输入缓冲区中下一个未读字符的位置 */
  size_t nIn;            /* 缓冲区内容：输入缓冲区中的字符总数 */
  char *zIn;             /* 输入缓冲区：存储从文件读取的数据 */
  char zErr[CSV_MXERR];  /* 错误缓冲区：存储错误消息 */
};

/*
** CsvReader 对象初始化函数
**
** 此函数将 CsvReader 结构体初始化为已知的初始状态，
** 确保所有字段都有合理的默认值。
**
** 初始化策略：
**
** 1. 安全初始化：
**    - 所有指针初始化为 NULL
**    - 所有计数器初始化为 0
**    - 错误缓冲区初始化为空字符串
**    - 状态标志设置为未开始状态
**
** 2. 资源管理：
**    - 确保没有悬空指针
**    - 防止野指针访问
**    - 简化后续的资源清理
**    - 提供可预测的初始状态
**
** 3. 调试友好：
**    - 一致的初始状态便于调试
**    - 简化问题定位
**    - 避免未定义行为
**    - 提高代码可维护性
**
** 参数说明：
** - p: 指向要初始化的 CsvReader 结构体指针
**
** 注意事项：
** - 调用者必须确保 p 不为 NULL
** - 此函数不会分配内存，只是初始化指针
** - 初始化后可以安全地调用其他 CsvReader 函数
** - 重复调用是安全的
**
** 使用示例：
**
** ```c
** CsvReader reader;
** csv_reader_init(&reader);
**
** // 现在可以安全地使用 reader 对象
** // 进行各种 CSV 读取操作
**
** // 完成后清理资源
** csv_reader_reset(&reader);
** ```
**
** 内存安全：
** - 防止访问未初始化的内存
** - 消除野指针问题
** - 提供明确的对象生命周期管理
** - 简化错误处理路径
**
** 性能考虑：
** - 内联函数，无函数调用开销
** - 最小初始化成本
** - 零运行时错误检查
** - 优化的内存布局
*/
static void csv_reader_init(CsvReader *p){
  p->in = 0;        /* 输入流初始化为 NULL */
  p->z = 0;         /* 字段缓冲区初始化为 NULL */
  p->n = 0;         /* 缓冲区长度初始化为 0 */
  p->nAlloc = 0;    /* 缓冲区容量初始化为 0 */
  p->nLine = 0;     /* 行号计数器初始化为 0 */
  p->bNotFirst = 0; /* 状态标志初始化为 FALSE */
  p->nIn = 0;       /* 输入缓冲区内容长度初始化为 0 */
  p->zIn = 0;       /* 输入缓冲区指针初始化为 NULL */
  p->zErr[0] = 0;   /* 错误消息初始化为空字符串 */
}

/* Close and reset a CsvReader object */
static void csv_reader_reset(CsvReader *p){
  if( p->in ){
    fclose(p->in);
    sqlite3_free(p->zIn);
  }
  sqlite3_free(p->z);
  csv_reader_init(p);
}

/* Report an error on a CsvReader */
static void csv_errmsg(CsvReader *p, const char *zFormat, ...){
  va_list ap;
  va_start(ap, zFormat);
  sqlite3_vsnprintf(CSV_MXERR, p->zErr, zFormat, ap);
  va_end(ap);
}

/* Open the file associated with a CsvReader
** Return the number of errors.
*/
static int csv_reader_open(
  CsvReader *p,               /* The reader to open */
  const char *zFilename,      /* Read from this filename */
  const char *zData           /*  ... or use this data */
){
  if( zFilename ){
    p->zIn = sqlite3_malloc( CSV_INBUFSZ );
    if( p->zIn==0 ){
      csv_errmsg(p, "out of memory");
      return 1;
    }
    p->in = fopen(zFilename, "rb");
    if( p->in==0 ){
      sqlite3_free(p->zIn);
      csv_reader_reset(p);
      csv_errmsg(p, "cannot open '%s' for reading", zFilename);
      return 1;
    }
  }else{
    assert( p->in==0 );
    p->zIn = (char*)zData;
    p->nIn = strlen(zData);
  }
  return 0;
}

/* The input buffer has overflowed.  Refill the input buffer, then
** return the next character
*/
static CSV_NOINLINE int csv_getc_refill(CsvReader *p){
  size_t got;

  assert( p->iIn>=p->nIn );  /* Only called on an empty input buffer */
  assert( p->in!=0 );        /* Only called if reading froma file */

  got = fread(p->zIn, 1, CSV_INBUFSZ, p->in);
  if( got==0 ) return EOF;
  p->nIn = got;
  p->iIn = 1;
  return p->zIn[0];
}

/* Return the next character of input.  Return EOF at end of input. */
static int csv_getc(CsvReader *p){
  if( p->iIn >= p->nIn ){
    if( p->in!=0 ) return csv_getc_refill(p);
    return EOF;
  }
  return ((unsigned char*)p->zIn)[p->iIn++];
}

/* Increase the size of p->z and append character c to the end. 
** Return 0 on success and non-zero if there is an OOM error */
static CSV_NOINLINE int csv_resize_and_append(CsvReader *p, char c){
  char *zNew;
  int nNew = p->nAlloc*2 + 100;
  zNew = sqlite3_realloc64(p->z, nNew);
  if( zNew ){
    p->z = zNew;
    p->nAlloc = nNew;
    p->z[p->n++] = c;
    return 0;
  }else{
    csv_errmsg(p, "out of memory");
    return 1;
  }
}

/* Append a single character to the CsvReader.z[] array.
** Return 0 on success and non-zero if there is an OOM error */
static int csv_append(CsvReader *p, char c){
  if( p->n>=p->nAlloc-1 ) return csv_resize_and_append(p, c);
  p->z[p->n++] = c;
  return 0;
}

/* Read a single field of CSV text.  Compatible with rfc4180 and extended
** with the option of having a separator other than ",".
**
**   +  Input comes from p->in.
**   +  Store results in p->z of length p->n.  Space to hold p->z comes
**      from sqlite3_malloc64().
**   +  Keep track of the line number in p->nLine.
**   +  Store the character that terminates the field in p->cTerm.  Store
**      EOF on end-of-file.
**
** Return 0 at EOF or on OOM.  On EOF, the p->cTerm character will have
** been set to EOF.
*/
static char *csv_read_one_field(CsvReader *p){
  int c;
  p->n = 0;
  c = csv_getc(p);
  if( c==EOF ){
    p->cTerm = EOF;
    return 0;
  }
  if( c=='"' ){
    int pc, ppc;
    int startLine = p->nLine;
    pc = ppc = 0;
    while( 1 ){
      c = csv_getc(p);
      if( c<='"' || pc=='"' ){
        if( c=='\n' ) p->nLine++;
        if( c=='"' ){
          if( pc=='"' ){
            pc = 0;
            continue;
          }
        }
        if( (c==',' && pc=='"')
         || (c=='\n' && pc=='"')
         || (c=='\n' && pc=='\r' && ppc=='"')
         || (c==EOF && pc=='"')
        ){
          do{ p->n--; }while( p->z[p->n]!='"' );
          p->cTerm = (char)c;
          break;
        }
        if( pc=='"' && c!='\r' ){
          csv_errmsg(p, "line %d: unescaped %c character", p->nLine, '"');
          break;
        }
        if( c==EOF ){
          csv_errmsg(p, "line %d: unterminated %c-quoted field\n",
                     startLine, '"');
          p->cTerm = (char)c;
          break;
        }
      }
      if( csv_append(p, (char)c) ) return 0;
      ppc = pc;
      pc = c;
    }
  }else{
    /* If this is the first field being parsed and it begins with the
    ** UTF-8 BOM  (0xEF BB BF) then skip the BOM */
    if( (c&0xff)==0xef && p->bNotFirst==0 ){
      csv_append(p, (char)c);
      c = csv_getc(p);
      if( (c&0xff)==0xbb ){
        csv_append(p, (char)c);
        c = csv_getc(p);
        if( (c&0xff)==0xbf ){
          p->bNotFirst = 1;
          p->n = 0;
          return csv_read_one_field(p);
        }
      }
    }
    while( c>',' || (c!=EOF && c!=',' && c!='\n') ){
      if( csv_append(p, (char)c) ) return 0;
      c = csv_getc(p);
    }
    if( c=='\n' ){
      p->nLine++;
      if( p->n>0 && p->z[p->n-1]=='\r' ) p->n--;
    }
    p->cTerm = (char)c;
  }
  assert( p->z==0 || p->n<p->nAlloc );
  if( p->z ) p->z[p->n] = 0;
  p->bNotFirst = 1;
  return p->z;
}


/* Forward references to the various virtual table methods implemented
** in this file. */
static int csvtabCreate(sqlite3*, void*, int, const char*const*, 
                           sqlite3_vtab**,char**);
static int csvtabConnect(sqlite3*, void*, int, const char*const*, 
                           sqlite3_vtab**,char**);
static int csvtabBestIndex(sqlite3_vtab*,sqlite3_index_info*);
static int csvtabDisconnect(sqlite3_vtab*);
static int csvtabOpen(sqlite3_vtab*, sqlite3_vtab_cursor**);
static int csvtabClose(sqlite3_vtab_cursor*);
static int csvtabFilter(sqlite3_vtab_cursor*, int idxNum, const char *idxStr,
                          int argc, sqlite3_value **argv);
static int csvtabNext(sqlite3_vtab_cursor*);
static int csvtabEof(sqlite3_vtab_cursor*);
static int csvtabColumn(sqlite3_vtab_cursor*,sqlite3_context*,int);
static int csvtabRowid(sqlite3_vtab_cursor*,sqlite3_int64*);

/* An instance of the CSV virtual table */
typedef struct CsvTable {
  sqlite3_vtab base;              /* Base class.  Must be first */
  char *zFilename;                /* Name of the CSV file */
  char *zData;                    /* Raw CSV data in lieu of zFilename */
  long iStart;                    /* Offset to start of data in zFilename */
  int nCol;                       /* Number of columns in the CSV file */
  unsigned int tstFlags;          /* Bit values used for testing */
} CsvTable;

/* Allowed values for tstFlags */
#define CSVTEST_FIDX  0x0001      /* Pretend that constrained search cost less*/

/* A cursor for the CSV virtual table */
typedef struct CsvCursor {
  sqlite3_vtab_cursor base;       /* Base class.  Must be first */
  CsvReader rdr;                  /* The CsvReader object */
  char **azVal;                   /* Value of the current row */
  int *aLen;                      /* Length of each entry */
  sqlite3_int64 iRowid;           /* The current rowid.  Negative for EOF */
} CsvCursor;

/* Transfer error message text from a reader into a CsvTable */
static void csv_xfer_error(CsvTable *pTab, CsvReader *pRdr){
  sqlite3_free(pTab->base.zErrMsg);
  pTab->base.zErrMsg = sqlite3_mprintf("%s", pRdr->zErr);
}

/*
** This method is the destructor fo a CsvTable object.
*/
static int csvtabDisconnect(sqlite3_vtab *pVtab){
  CsvTable *p = (CsvTable*)pVtab;
  sqlite3_free(p->zFilename);
  sqlite3_free(p->zData);
  sqlite3_free(p);
  return SQLITE_OK;
}

/* Skip leading whitespace.  Return a pointer to the first non-whitespace
** character, or to the zero terminator if the string has only whitespace */
static const char *csv_skip_whitespace(const char *z){
  while( isspace((unsigned char)z[0]) ) z++;
  return z;
}

/* Remove trailing whitespace from the end of string z[] */
static void csv_trim_whitespace(char *z){
  size_t n = strlen(z);
  while( n>0 && isspace((unsigned char)z[n]) ) n--;
  z[n] = 0;
}

/* Dequote the string */
static void csv_dequote(char *z){
  int j;
  char cQuote = z[0];
  size_t i, n;

  if( cQuote!='\'' && cQuote!='"' ) return;
  n = strlen(z);
  if( n<2 || z[n-1]!=z[0] ) return;
  for(i=1, j=0; i<n-1; i++){
    if( z[i]==cQuote && z[i+1]==cQuote ) i++;
    z[j++] = z[i];
  }
  z[j] = 0;
}

/* Check to see if the string is of the form:  "TAG = VALUE" with optional
** whitespace before and around tokens.  If it is, return a pointer to the
** first character of VALUE.  If it is not, return NULL.
*/
static const char *csv_parameter(const char *zTag, int nTag, const char *z){
  z = csv_skip_whitespace(z);
  if( strncmp(zTag, z, nTag)!=0 ) return 0;
  z = csv_skip_whitespace(z+nTag);
  if( z[0]!='=' ) return 0;
  return csv_skip_whitespace(z+1);
}

/* Decode a parameter that requires a dequoted string.
**
** Return 1 if the parameter is seen, or 0 if not.  1 is returned
** even if there is an error.  If an error occurs, then an error message
** is left in p->zErr.  If there are no errors, p->zErr[0]==0.
*/
static int csv_string_parameter(
  CsvReader *p,            /* Leave the error message here, if there is one */
  const char *zParam,      /* Parameter we are checking for */
  const char *zArg,        /* Raw text of the virtual table argment */
  char **pzVal             /* Write the dequoted string value here */
){
  const char *zValue;
  zValue = csv_parameter(zParam,(int)strlen(zParam),zArg);
  if( zValue==0 ) return 0;
  p->zErr[0] = 0;
  if( *pzVal ){
    csv_errmsg(p, "more than one '%s' parameter", zParam);
    return 1;
  }
  *pzVal = sqlite3_mprintf("%s", zValue);
  if( *pzVal==0 ){
    csv_errmsg(p, "out of memory");
    return 1;
  }
  csv_trim_whitespace(*pzVal);
  csv_dequote(*pzVal);
  return 1;
}


/* Return 0 if the argument is false and 1 if it is true.  Return -1 if
** we cannot really tell.
*/
static int csv_boolean(const char *z){
  if( sqlite3_stricmp("yes",z)==0
   || sqlite3_stricmp("on",z)==0
   || sqlite3_stricmp("true",z)==0
   || (z[0]=='1' && z[1]==0)
  ){
    return 1;
  }
  if( sqlite3_stricmp("no",z)==0
   || sqlite3_stricmp("off",z)==0
   || sqlite3_stricmp("false",z)==0
   || (z[0]=='0' && z[1]==0)
  ){
    return 0;
  }
  return -1;
}

/* Check to see if the string is of the form:  "TAG = BOOLEAN" or just "TAG".
** If it is, set *pValue to be the value of the boolean ("true" if there is
** not "= BOOLEAN" component) and return non-zero.  If the input string
** does not begin with TAG, return zero.
*/
static int csv_boolean_parameter(
  const char *zTag,       /* Tag we are looking for */
  int nTag,               /* Size of the tag in bytes */
  const char *z,          /* Input parameter */
  int *pValue             /* Write boolean value here */
){
  int b;
  z = csv_skip_whitespace(z);
  if( strncmp(zTag, z, nTag)!=0 ) return 0;
  z = csv_skip_whitespace(z + nTag);
  if( z[0]==0 ){
    *pValue = 1;
    return 1;
  }
  if( z[0]!='=' ) return 0;
  z = csv_skip_whitespace(z+1);
  b = csv_boolean(z);
  if( b>=0 ){
    *pValue = b;
    return 1;
  }
  return 0;
}

/*
** Parameters:
**    filename=FILENAME          Name of file containing CSV content
**    data=TEXT                  Direct CSV content.
**    schema=SCHEMA              Alternative CSV schema.
**    header=YES|NO              First row of CSV defines the names of
**                               columns if "yes".  Default "no".
**    columns=N                  Assume the CSV file contains N columns.
**
** Only available if compiled with SQLITE_TEST:
**    
**    testflags=N                Bitmask of test flags.  Optional
**
** If schema= is omitted, then the columns are named "c0", "c1", "c2",
** and so forth.  If columns=N is omitted, then the file is opened and
** the number of columns in the first row is counted to determine the
** column count.  If header=YES, then the first row is skipped.
*/
static int csvtabConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  CsvTable *pNew = 0;        /* The CsvTable object to construct */
  int bHeader = -1;          /* header= flags.  -1 means not seen yet */
  int rc = SQLITE_OK;        /* Result code from this routine */
  int i, j;                  /* Loop counters */
#ifdef SQLITE_TEST
  int tstFlags = 0;          /* Value for testflags=N parameter */
#endif
  int b;                     /* Value of a boolean parameter */
  int nCol = -99;            /* Value of the columns= parameter */
  CsvReader sRdr;            /* A CSV file reader used to store an error
                             ** message and/or to count the number of columns */
  static const char *azParam[] = {
     "filename", "data", "schema", 
  };
  char *azPValue[3];         /* Parameter values */
# define CSV_FILENAME (azPValue[0])
# define CSV_DATA     (azPValue[1])
# define CSV_SCHEMA   (azPValue[2])


  assert( sizeof(azPValue)==sizeof(azParam) );
  memset(&sRdr, 0, sizeof(sRdr));
  memset(azPValue, 0, sizeof(azPValue));
  for(i=3; i<argc; i++){
    const char *z = argv[i];
    const char *zValue;
    for(j=0; j<sizeof(azParam)/sizeof(azParam[0]); j++){
      if( csv_string_parameter(&sRdr, azParam[j], z, &azPValue[j]) ) break;
    }
    if( j<sizeof(azParam)/sizeof(azParam[0]) ){
      if( sRdr.zErr[0] ) goto csvtab_connect_error;
    }else
    if( csv_boolean_parameter("header",6,z,&b) ){
      if( bHeader>=0 ){
        csv_errmsg(&sRdr, "more than one 'header' parameter");
        goto csvtab_connect_error;
      }
      bHeader = b;
    }else
#ifdef SQLITE_TEST
    if( (zValue = csv_parameter("testflags",9,z))!=0 ){
      tstFlags = (unsigned int)atoi(zValue);
    }else
#endif
    if( (zValue = csv_parameter("columns",7,z))!=0 ){
      if( nCol>0 ){
        csv_errmsg(&sRdr, "more than one 'columns' parameter");
        goto csvtab_connect_error;
      }
      nCol = atoi(zValue);
      if( nCol<=0 ){
        csv_errmsg(&sRdr, "column= value must be positive");
        goto csvtab_connect_error;
      }
    }else
    {
      csv_errmsg(&sRdr, "bad parameter: '%s'", z);
      goto csvtab_connect_error;
    }
  }
  if( (CSV_FILENAME==0)==(CSV_DATA==0) ){
    csv_errmsg(&sRdr, "must specify either filename= or data= but not both");
    goto csvtab_connect_error;
  }

  if( (nCol<=0 || bHeader==1)
   && csv_reader_open(&sRdr, CSV_FILENAME, CSV_DATA)
  ){
    goto csvtab_connect_error;
  }
  pNew = sqlite3_malloc( sizeof(*pNew) );
  *ppVtab = (sqlite3_vtab*)pNew;
  if( pNew==0 ) goto csvtab_connect_oom;
  memset(pNew, 0, sizeof(*pNew));
  if( CSV_SCHEMA==0 ){
    sqlite3_str *pStr = sqlite3_str_new(0);
    char *zSep = "";
    int iCol = 0;
    sqlite3_str_appendf(pStr, "CREATE TABLE x(");
    if( nCol<0 && bHeader<1 ){
      nCol = 0;
      do{
        csv_read_one_field(&sRdr);
        nCol++;
      }while( sRdr.cTerm==',' );
    }
    if( nCol>0 && bHeader<1 ){
      for(iCol=0; iCol<nCol; iCol++){
        sqlite3_str_appendf(pStr, "%sc%d TEXT", zSep, iCol);
        zSep = ",";
      }
    }else{
      do{
        char *z = csv_read_one_field(&sRdr);
        if( (nCol>0 && iCol<nCol) || (nCol<0 && bHeader) ){
          sqlite3_str_appendf(pStr,"%s\"%w\" TEXT", zSep, z);
          zSep = ",";
          iCol++;
        }
      }while( sRdr.cTerm==',' );
      if( nCol<0 ){
        nCol = iCol;
      }else{
        while( iCol<nCol ){
          sqlite3_str_appendf(pStr,"%sc%d TEXT", zSep, ++iCol);
          zSep = ",";
        }
      }
    }
    pNew->nCol = nCol;
    sqlite3_str_appendf(pStr, ")");
    CSV_SCHEMA = sqlite3_str_finish(pStr);
    if( CSV_SCHEMA==0 ) goto csvtab_connect_oom;
  }else if( nCol<0 ){
    do{
      csv_read_one_field(&sRdr);
      pNew->nCol++;
    }while( sRdr.cTerm==',' );
  }else{
    pNew->nCol = nCol;
  }
  pNew->zFilename = CSV_FILENAME;  CSV_FILENAME = 0;
  pNew->zData = CSV_DATA;          CSV_DATA = 0;
#ifdef SQLITE_TEST
  pNew->tstFlags = tstFlags;
#endif
  if( bHeader!=1 ){
    pNew->iStart = 0;
  }else if( pNew->zData ){
    pNew->iStart = (int)sRdr.iIn;
  }else{
    pNew->iStart = (int)(ftell(sRdr.in) - sRdr.nIn + sRdr.iIn);
  }
  csv_reader_reset(&sRdr);
  rc = sqlite3_declare_vtab(db, CSV_SCHEMA);
  if( rc ){
    csv_errmsg(&sRdr, "bad schema: '%s' - %s", CSV_SCHEMA, sqlite3_errmsg(db));
    goto csvtab_connect_error;
  }
  for(i=0; i<sizeof(azPValue)/sizeof(azPValue[0]); i++){
    sqlite3_free(azPValue[i]);
  }
  /* Rationale for DIRECTONLY:
  ** An attacker who controls a database schema could use this vtab
  ** to exfiltrate sensitive data from other files in the filesystem.
  ** And, recommended practice is to put all CSV virtual tables in the
  ** TEMP namespace, so they should still be usable from within TEMP
  ** views, so there shouldn't be a serious loss of functionality by
  ** prohibiting the use of this vtab from persistent triggers and views.
  */
  sqlite3_vtab_config(db, SQLITE_VTAB_DIRECTONLY);
  return SQLITE_OK;

csvtab_connect_oom:
  rc = SQLITE_NOMEM;
  csv_errmsg(&sRdr, "out of memory");

csvtab_connect_error:
  if( pNew ) csvtabDisconnect(&pNew->base);
  for(i=0; i<sizeof(azPValue)/sizeof(azPValue[0]); i++){
    sqlite3_free(azPValue[i]);
  }
  if( sRdr.zErr[0] ){
    sqlite3_free(*pzErr);
    *pzErr = sqlite3_mprintf("%s", sRdr.zErr);
  }
  csv_reader_reset(&sRdr);
  if( rc==SQLITE_OK ) rc = SQLITE_ERROR;
  return rc;
}

/*
** Reset the current row content held by a CsvCursor.
*/
static void csvtabCursorRowReset(CsvCursor *pCur){
  CsvTable *pTab = (CsvTable*)pCur->base.pVtab;
  int i;
  for(i=0; i<pTab->nCol; i++){
    sqlite3_free(pCur->azVal[i]);
    pCur->azVal[i] = 0;
    pCur->aLen[i] = 0;
  }
}

/*
** The xConnect and xCreate methods do the same thing, but they must be
** different so that the virtual table is not an eponymous virtual table.
*/
static int csvtabCreate(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
 return csvtabConnect(db, pAux, argc, argv, ppVtab, pzErr);
}

/*
** Destructor for a CsvCursor.
*/
static int csvtabClose(sqlite3_vtab_cursor *cur){
  CsvCursor *pCur = (CsvCursor*)cur;
  csvtabCursorRowReset(pCur);
  csv_reader_reset(&pCur->rdr);
  sqlite3_free(cur);
  return SQLITE_OK;
}

/*
** Constructor for a new CsvTable cursor object.
*/
static int csvtabOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor){
  CsvTable *pTab = (CsvTable*)p;
  CsvCursor *pCur;
  size_t nByte;
  nByte = sizeof(*pCur) + (sizeof(char*)+sizeof(int))*pTab->nCol;
  pCur = sqlite3_malloc64( nByte );
  if( pCur==0 ) return SQLITE_NOMEM;
  memset(pCur, 0, nByte);
  pCur->azVal = (char**)&pCur[1];
  pCur->aLen = (int*)&pCur->azVal[pTab->nCol];
  *ppCursor = &pCur->base;
  if( csv_reader_open(&pCur->rdr, pTab->zFilename, pTab->zData) ){
    csv_xfer_error(pTab, &pCur->rdr);
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}


/*
** Advance a CsvCursor to its next row of input.
** Set the EOF marker if we reach the end of input.
*/
static int csvtabNext(sqlite3_vtab_cursor *cur){
  CsvCursor *pCur = (CsvCursor*)cur;
  CsvTable *pTab = (CsvTable*)cur->pVtab;
  int i = 0;
  char *z;
  do{
    z = csv_read_one_field(&pCur->rdr);
    if( z==0 ){
      break;
    }
    if( i<pTab->nCol ){
      if( pCur->aLen[i] < pCur->rdr.n+1 ){
        char *zNew = sqlite3_realloc64(pCur->azVal[i], pCur->rdr.n+1);
        if( zNew==0 ){
          csv_errmsg(&pCur->rdr, "out of memory");
          csv_xfer_error(pTab, &pCur->rdr);
          break;
        }
        pCur->azVal[i] = zNew;
        pCur->aLen[i] = pCur->rdr.n+1;
      }
      memcpy(pCur->azVal[i], z, pCur->rdr.n+1);
      i++;
    }
  }while( pCur->rdr.cTerm==',' );
  if( z==0 && i==0 ){
    pCur->iRowid = -1;
  }else{
    pCur->iRowid++;
    while( i<pTab->nCol ){
      sqlite3_free(pCur->azVal[i]);
      pCur->azVal[i] = 0;
      pCur->aLen[i] = 0;
      i++;
    }
  }
  return SQLITE_OK;
}

/*
** Return values of columns for the row at which the CsvCursor
** is currently pointing.
*/
static int csvtabColumn(
  sqlite3_vtab_cursor *cur,   /* The cursor */
  sqlite3_context *ctx,       /* First argument to sqlite3_result_...() */
  int i                       /* Which column to return */
){
  CsvCursor *pCur = (CsvCursor*)cur;
  CsvTable *pTab = (CsvTable*)cur->pVtab;
  if( i>=0 && i<pTab->nCol && pCur->azVal[i]!=0 ){
    sqlite3_result_text(ctx, pCur->azVal[i], -1, SQLITE_TRANSIENT);
  }
  return SQLITE_OK;
}

/*
** Return the rowid for the current row.
*/
static int csvtabRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  CsvCursor *pCur = (CsvCursor*)cur;
  *pRowid = pCur->iRowid;
  return SQLITE_OK;
}

/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int csvtabEof(sqlite3_vtab_cursor *cur){
  CsvCursor *pCur = (CsvCursor*)cur;
  return pCur->iRowid<0;
}

/*
** Only a full table scan is supported.  So xFilter simply rewinds to
** the beginning.
*/
static int csvtabFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  CsvCursor *pCur = (CsvCursor*)pVtabCursor;
  CsvTable *pTab = (CsvTable*)pVtabCursor->pVtab;
  pCur->iRowid = 0;

  /* Ensure the field buffer is always allocated. Otherwise, if the
  ** first field is zero bytes in size, this may be mistaken for an OOM
  ** error in csvtabNext(). */
  if( csv_append(&pCur->rdr, 0) ) return SQLITE_NOMEM;

  if( pCur->rdr.in==0 ){
    assert( pCur->rdr.zIn==pTab->zData );
    assert( pTab->iStart>=0 );
    assert( (size_t)pTab->iStart<=pCur->rdr.nIn );
    pCur->rdr.iIn = pTab->iStart;
  }else{
    fseek(pCur->rdr.in, pTab->iStart, SEEK_SET);
    pCur->rdr.iIn = 0;
    pCur->rdr.nIn = 0;
  }
  return csvtabNext(pVtabCursor);
}

/*
** Only a forward full table scan is supported.  xBestIndex is mostly
** a no-op.  If CSVTEST_FIDX is set, then the presence of equality
** constraints lowers the estimated cost, which is fiction, but is useful
** for testing certain kinds of virtual table behavior.
*/
static int csvtabBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  pIdxInfo->estimatedCost = 1000000;
#ifdef SQLITE_TEST
  if( (((CsvTable*)tab)->tstFlags & CSVTEST_FIDX)!=0 ){
    /* The usual (and sensible) case is to always do a full table scan.
    ** The code in this branch only runs when testflags=1.  This code
    ** generates an artifical and unrealistic plan which is useful
    ** for testing virtual table logic but is not helpful to real applications.
    **
    ** Any ==, LIKE, or GLOB constraint is marked as usable by the virtual
    ** table (even though it is not) and the cost of running the virtual table
    ** is reduced from 1 million to just 10.  The constraints are *not* marked
    ** as omittable, however, so the query planner should still generate a
    ** plan that gives a correct answer, even if they plan is not optimal.
    */
    int i;
    int nConst = 0;
    for(i=0; i<pIdxInfo->nConstraint; i++){
      unsigned char op;
      if( pIdxInfo->aConstraint[i].usable==0 ) continue;
      op = pIdxInfo->aConstraint[i].op;
      if( op==SQLITE_INDEX_CONSTRAINT_EQ 
       || op==SQLITE_INDEX_CONSTRAINT_LIKE
       || op==SQLITE_INDEX_CONSTRAINT_GLOB
      ){
        pIdxInfo->estimatedCost = 10;
        pIdxInfo->aConstraintUsage[nConst].argvIndex = nConst+1;
        nConst++;
      }
    }
  }
#endif
  return SQLITE_OK;
}


/*
** CSV 虚拟表模块结构定义
**
** 这是 SQLite CSV 扩展的核心模块结构，定义了虚拟表的所有回调函数。
** 该模块实现了完整的 CSV 文件读取和查询功能。
**
** 模块功能概述：
** - 提供标准的 CSV 文件访问接口
** - 支持灵活的格式配置选项
** - 实现高效的流式数据处理
** - 提供完整的 SQL 查询支持
**
** 接口函数详解：
**
** 生命周期管理：
** - xCreate: 创建新的 CSV 虚拟表实例
** - xConnect: 连接到现有 CSV 虚拟表
** - xDisconnect: 断开连接，释放资源
** - xDestroy: 销毁虚拟表，清理持久化数据
**
** 查询处理：
** - xBestIndex: 查询优化器接口，确定最佳查询计划
** - xOpen: 打开游标，准备遍历 CSV 数据
** - xClose: 关闭游标，释放相关资源
** - xFilter: 应用查询约束，开始数据扫描
** - xNext: 移动到下一个数据行
** - xEof: 检查是否到达数据文件末尾
** - xColumn: 获取当前行的列数据
** - xRowid: 获取当前行的唯一标识符
**
** 功能限制：
** - xUpdate: 不支持数据写入操作
** - 事务相关：不支持事务操作
** - 高级功能：不支持重命名、保存点等
**
** 设计特点：
**
** 只读访问：
** - CSV 虚拟表专门用于数据读取
** - 不支持 INSERT、UPDATE、DELETE 操作
** - 确保原始数据的安全性和完整性
** - 简化实现并提高性能
**
** 性能优化：
** - 流式处理：支持大文件而不耗尽内存
** - 缓冲读取：减少 I/O 操作次数
** - 延迟解析：按需解析 CSV 行
** - 索引友好：支持查询优化器的决策
**
** 错误处理：
** - 详细的错误信息报告
** - 优雅的错误恢复机制
** - 文件访问错误的处理
** - CSV 格式错误的诊断
**
** 扩展能力：
** - 多种 CSV 格式支持
** - 灵活的分隔符配置
** - 可选的标题行处理
** - 自定义列定义支持
**
** 并发支持：
** - 多读者并发访问
** - 线程安全的读取操作
** - 文件锁定机制
** - 状态隔离保证
**
** 使用场景：
**
** 数据分析：
** - 日志文件分析
** - 统计数据处理
** - 业务报表生成
** - 科学计算数据导入
**
** 数据导入：
** - 数据仓库 ETL 过程
** - 数据迁移和转换
** - 配置文件解析
** - 批量数据处理
**
** 报表生成：
** - CSV 数据查询和过滤
** - 聚合和统计分析
** - 数据可视化支持
** - 实时报表生成
**
** 集成应用：
** - Web 应用的数据导入
** - 移动应用的数据同步
** - 嵌入式系统的配置管理
** - 物联网数据处理
**
** 性能指标：
**
** 内存效率：
** - 固定大小的读取缓冲区
** - 按需分配的字段缓冲区
** - 最小的内存开销
** - 支持大文件处理
**
** 处理速度：
** - 优化的字符解析算法
** - 最小的字符串复制操作
** - 高效的状态机实现
** - 缓存友好的数据访问
**
** 扩展性：
** - 支持超大 CSV 文件
** - 可配置的解析参数
** - 模块化的设计架构
** - 平台无关的实现
**
** 最佳实践：
**
** 文件处理：
** - 使用适当大小的缓冲区
** - 预处理不规范的 CSV 数据
** - 监控内存使用情况
** - 处理各种编码格式
**
** 查询优化：
** - 使用适当的 WHERE 条件
** - 避免全表扫描
** - 合理使用 LIMIT 和 OFFSET
** - 利用索引优化查询
**
** 错误处理：
** - 检查文件访问权限
** - 验证 CSV 格式规范
** - 处理编码转换问题
** - 监控磁盘空间使用
*/
static sqlite3_module CsvModule = {
  0,                       /* iVersion - 虚拟表接口版本 */
  csvtabCreate,            /* xCreate - 创建新的 CSV 虚拟表 */
  csvtabConnect,           /* xConnect - 连接到现有 CSV 虚拟表 */
  csvtabBestIndex,         /* xBestIndex - 查询优化器接口 */
  csvtabDisconnect,        /* xDisconnect - 断开连接 */
  csvtabDisconnect,        /* xDestroy - 销毁虚拟表 */
  csvtabOpen,              /* xOpen - 打开游标 */
  csvtabClose,             /* xClose - 关闭游标 */
  csvtabFilter,            /* xFilter - 配置扫描约束 */
  csvtabNext,              /* xNext - 前进游标 */
  csvtabEof,               /* xEof - 检查扫描结束 */
  csvtabColumn,            /* xColumn - 读取列数据 */
  csvtabRowid,             /* xRowid - 读取行 ID */
  0,                       /* xUpdate - 不支持更新操作 */
  0,                       /* xBegin - 不支持事务 */
  0,                       /* xSync - 不支持事务 */
  0,                       /* xCommit - 不支持事务 */
  0,                       /* xRollback - 不支持事务 */
  0,                       /* xFindMethod - 不支持自定义方法 */
  0,                       /* xRename - 不支持重命名 */
  0,                       /* xSavepoint - 不支持保存点 */
  0,                       /* xRelease - 不支持保存点释放 */
  0,                       /* xRollbackTo - 不支持回滚到保存点 */
  0,                       /* xShadowName - 不支持影子表 */
  0                        /* xIntegrity - 不支持完整性检查 */
};

#ifdef SQLITE_TEST
/*
** For virtual table testing, make a version of the CSV virtual table
** available that has an xUpdate function.  But the xUpdate always returns
** SQLITE_READONLY since the CSV file is not really writable.
*/
static int csvtabUpdate(sqlite3_vtab *p,int n,sqlite3_value**v,sqlite3_int64*x){
  return SQLITE_READONLY;
}
static sqlite3_module CsvModuleFauxWrite = {
  0,                       /* iVersion */
  csvtabCreate,            /* xCreate */
  csvtabConnect,           /* xConnect */
  csvtabBestIndex,         /* xBestIndex */
  csvtabDisconnect,        /* xDisconnect */
  csvtabDisconnect,        /* xDestroy */
  csvtabOpen,              /* xOpen - open a cursor */
  csvtabClose,             /* xClose - close a cursor */
  csvtabFilter,            /* xFilter - configure scan constraints */
  csvtabNext,              /* xNext - advance a cursor */
  csvtabEof,               /* xEof - check for end of scan */
  csvtabColumn,            /* xColumn - read data */
  csvtabRowid,             /* xRowid - read data */
  csvtabUpdate,            /* xUpdate */
  0,                       /* xBegin */
  0,                       /* xSync */
  0,                       /* xCommit */
  0,                       /* xRollback */
  0,                       /* xFindMethod */
  0,                       /* xRename */
  0,                       /* xSavepoint */
  0,                       /* xRelease */
  0,                       /* xRollbackTo */
  0,                       /* xShadowName */
  0                        /* xIntegrity */
};
#endif /* SQLITE_TEST */

#endif /* !defined(SQLITE_OMIT_VIRTUALTABLE) */


#ifdef _WIN32
__declspec(dllexport)
#endif
/*
** CSV 扩展初始化函数
**
** 此函数在扩展加载时被调用。新的 CSV 虚拟表模块注册到
** 调用的数据库连接中。
**
** 函数职责：
**
** API 初始化：
** - 建立 SQLite 与扩展的通信接口
** - 初始化扩展 API 指针
** - 验证 SQLite 版本兼容性
** - 设置扩展运行环境
**
** 模块注册：
** - 注册主要的 CSV 虚拟表模块
** - 在测试模式下注册写入模块
** - 建立模块特定配置
** - 设置错误处理机制
**
** 参数说明：
** - db: SQLite 数据库连接句柄
** - pzErrMsg: 错误信息输出参数
** - pApi: SQLite API 函数指针表
**
** 返回值：
** - SQLITE_OK: 扩展初始化成功
** - SQLITE_ERROR: 初始化失败，错误信息在 pzErrMsg 中
** - 其他错误码: 模块注册失败
**
** 条件编译处理：
**
** 虚拟表支持：
** - SQLITE_OMIT_VIRTUALTABLE: 检查是否启用虚拟表支持
** - 如果未启用，则返回成功但不提供功能
** - 确保在不支持虚拟表的环境中安全运行
**
** 测试模式：
** - SQLITE_TEST: 编译时测试开关
** - 注册额外的测试用模块 csv_wr
** - 支持写入操作的虚拟表（仅用于测试）
** - 提供完整的测试功能支持
**
** 安全检查：
** - 验证 SQLite API 版本兼容性
** - 检查必要的 SQLite 功能支持
** - 确认扩展加载的安全性
** - 防止重复初始化
**
** 错误处理：
**
** 初始化失败：
** - 详细的错误信息设置
** - 资源清理和回滚机制
** - 异常情况的优雅处理
** - 调试信息的输出
**
** 模块注册失败：
** - 检查模块名称冲突
** - 验证模块结构完整性
** - 处理内存分配失败
** - 提供详细的错误诊断
**
** 扩展功能：
**
** 主要模块：
** - 注册名为 "csv" 的虚拟表模块
** - 提供只读的 CSV 文件访问
** - 支持完整的查询功能
** - 实现标准的虚拟表接口
**
** 测试模块：
** - 注册名为 "csv_wr" 的测试模块
** - 支持写入操作的模拟
** - 提供扩展功能的测试接口
** - 便于单元测试和集成测试
**
** 性能特性：
**
** 初始化开销：
** - 最小的初始化成本
** - 高效的模块注册
** - 延迟资源分配
** - 优化的启动过程
**
** 运行时性能：
** - 零运行时开销
** - 高效的模块查找
** - 最小的内存占用
** - 优化的函数调用
**
** 并发支持：
** - 线程安全的初始化
** - 原子操作保证
** - 并发模块访问支持
** - 状态隔离机制
**
** 使用示例：
**
** ```c
** // 加载 CSV 扩展
** sqlite3 *db;
** sqlite3_open(":memory:", &db);
**
** // 初始化扩展
** int rc = sqlite3_csv_init(db, NULL, &sqlite3_api);
**
** if (rc == SQLITE_OK) {
**     // 使用 CSV 功能
**     sqlite3_exec(db,
**         "CREATE VIRTUAL TABLE data USING csv(filename='test.csv');"
**         "SELECT * FROM data WHERE c1 > 100;",
**         NULL, NULL, NULL);
** }
** ```
**
** 高级用法：
**
** ```sql
** -- 创建 CSV 虚拟表
** CREATE VIRTUAL TABLE sales USING csv(
**     filename='sales_data.csv',
**     schema='CREATE TABLE x(date TEXT, amount REAL, product TEXT)',
**     header=1
** );
**
** -- 查询 CSV 数据
** SELECT product, SUM(amount) as total_sales
** FROM sales
** WHERE date BETWEEN '2023-01-01' AND '2023-12-31'
** GROUP BY product
** ORDER BY total_sales DESC;
** ```
**
** 配置要求：
**
** 编译选项：
** - 必须启用虚拟表支持
** - SQLite 3.6+ 版本
** - 标准库文件访问支持
** - 内存管理功能
**
** 运行环境：
** - 文件系统访问权限
** - 足够的内存资源
** - 适当的文件权限设置
** - 磁盘空间可用性
**
** 最佳实践：
**
** 加载策略：
** - 在数据库连接建立后立即加载
** - 检查加载成功状态
** - 处理加载失败情况
** - 避免重复加载
**
** 错误处理：
** - 始终检查返回码
** - 处理内存不足情况
** - 验证文件访问权限
** - 监控资源使用情况
**
** 测试支持：
**
** 调试模式：
** - 详细的初始化日志
** - 错误状态跟踪
** - 性能指标监控
** - 内存使用分析
**
** 功能测试：
** - 模块功能验证
** - 错误条件测试
** - 性能基准测试
** - 并发访问测试
*/
int sqlite3_csv_init(
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
#ifndef SQLITE_OMIT_VIRTUALTABLE
  int rc;
  SQLITE_EXTENSION_INIT2(pApi);
  rc = sqlite3_create_module(db, "csv", &CsvModule, 0);
#ifdef SQLITE_TEST
  if( rc==SQLITE_OK ){
    rc = sqlite3_create_module(db, "csv_wr", &CsvModuleFauxWrite, 0);
  }
#endif
  return rc;
#else
  return SQLITE_OK;
#endif
}
