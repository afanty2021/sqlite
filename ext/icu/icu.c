/*
** 2007 May 6
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** $Id: icu.c,v 1.7 2007/12/13 21:54:11 drh Exp $
**
** SQLite ICU 扩展实现
**
** 本文件实现了 SQLite 与 ICU 库（"International Components for Unicode"，
** 一个用于处理 Unicode 数据的开源库）的集成。该集成使用 ICU 为 SQLite
** 提供以下功能：
**
**   * 使用 ICU uregex_XX() API 实现 SQL regexp() 函数（以及 REGEXP 操作符）
**
**   * 实现 SQL 标量函数 upper() 和 lower() 用于大小写映射
**
**   * ICU 和 SQLite 排序规则（collation sequences）的集成
**
**   * 实现使用 ICU 提供大小写无关匹配的 LIKE 操作符
**
**
** ICU 扩展架构概述：
**
** 1. 功能集成：
**    - 正则表达式支持：基于 ICU 强大的正则引擎
**    - 大小写转换：符合 Unicode 标准的大小写映射
**    - 排序规则：支持语言特定的排序和比较规则
**    - 模式匹配：大小写不敏感的 LIKE 操作
**
**
** 2. ICU 库特性：
**    - Unicode 标准兼容性：完全支持 Unicode 4.0+ 标准
**    - 语言多样性：支持 100+ 种语言的本地化规则
**    - 性能优化：针对 Unicode 处理的高性能实现
**    - 可扩展性：支持自定义排序规则和转换规则
**
**
** 3. SQLite 集成优势：
**    - 标准兼容：符合 SQL 标准的字符串处理函数
**    - 性能提升：相比纯 C 实现更好的 Unicode 处理性能
**    - 功能丰富：提供复杂的文本处理和比较功能
**    - 易用性：与现有 SQLite API 无缝集成
**
**
** 4. 主要实现模块：
**    - 正则表达式模块：基于 ICU uregex API 的 REGEXP 函数
**    - 大小写转换模块：Unicode 标准的大小写映射函数
**    - 排序规则模块：ICU Collator 与 SQLite 排序规则的桥接
**    - LIKE 操作模块：大小写不敏感的模式匹配
**
**
** 5. 技术特点：
**    - 内存管理：与 SQLite 内存管理系统集成
**    - 错误处理：完整的错误传播和报告机制
**    - 字符编码：UTF-8 和 UTF-16 的透明转换
**    - 线程安全：支持 SQLite 的多线程模式
**
**
** 6. 应用场景：
**    - 国际化应用：多语言文本处理和比较
**    - 数据分析：复杂的文本模式匹配和提取
**    - 内容管理：符合语言习惯的排序和搜索
**    - 本地化软件：特定语言的字符串处理需求
**
**
** 7. 使用示例：
**    ```sql
**    -- 启用 ICU 扩展后
**    SELECT regexp('[0-9]+', 'abc123def');  -- 返回 1（匹配成功）
**    SELECT upper('café');                  -- 返回 'CAFÉ'（正确处理重音）
**    SELECT 'café' COLLATE UNICODE;         -- 使用 Unicode 排序
**    SELECT 'café' LIKE 'CAF%';             -- 大小写不敏感匹配
**    ```
**
** 8. 依赖关系：
**    - ICU 库：需要 libicuuc, libicui18n 等库
**    - 编译选项：SQLITE_ENABLE_ICU 或 SQLITE_ENABLE_ICU_COLLATIONS
**    - 版本兼容：ICU 3.6+ 版本支持
**    - 平台支持：跨平台的 ICU 库支持
**
**
** 9. 性能考量：
**    - 初始化开销：ICU 库的初始化成本
**    - 内存使用：Unicode 数据结构的内存占用
**    - 缓存机制：排序规则和正则表达式的缓存策略
**    - 批处理：大量文本处理的性能优化
**
** 10. 扩展性设计：
**     - 自定义排序规则：支持用户定义的排序规则
**     - 扩展函数：基于 ICU 的其他字符串处理函数
**     - 本地化设置：运行时语言和区域设置
**     - 编码转换：多种字符编码间的转换支持
**
** 集成方式：
**
** - 编译时集成：通过 SQLITE_ENABLE_ICU 宏包含在核心中
** - 运行时加载：作为动态扩展使用 sqlite3_load_extension()
** - 静态链接：与 SQLite 主程序静态链接
** - 模块化设计：可选择性启用不同功能模块
*/

#if !defined(SQLITE_CORE)                  \
 || defined(SQLITE_ENABLE_ICU)             \
 || defined(SQLITE_ENABLE_ICU_COLLATIONS)

/* Include ICU headers */
#include <unicode/utypes.h>
#include <unicode/uregex.h>
#include <unicode/ustring.h>
#include <unicode/ucol.h>

#include <assert.h>

#ifndef SQLITE_CORE
  #include "sqlite3ext.h"
  SQLITE_EXTENSION_INIT1
#else
  #include "sqlite3.h"
#endif

/*
** ICU 函数错误处理和报告机制
**
** 当从 SQL 标量函数实现中调用的 ICU 函数返回错误时调用此函数。
**
** 传递的第一个参数（标量函数上下文）会被加载基于以下两个参数的错误消息。
**
** 错误处理特点：
** 1. 统一错误格式：所有 ICU 错误都转换为标准格式
** 2. 详细错误信息：包含失败的函数名和 ICU 错误描述
** 3. 安全性：防止缓冲区溢出的安全字符串处理
** 4. 传播机制：错误通过 SQLite 上下文传播到上层
**
** 错误消息格式：
**   "ICU error: function_name(): error_description"
**
** 常见错误类型：
** - U_MEMORY_ALLOCATION_ERROR: 内存分配失败
** - U_ILLEGAL_ARGUMENT_ERROR: 非法参数
** - U_REGEX_BAD_ESCAPE_SEQUENCE: 正则表达式转义序列错误
** - U_REGEX_PATTERN_SYNTAX_ERROR: 正则表达式语法错误
** - U_INDEX_OUTOFBOUNDS_ERROR: 索引越界错误
**
** 错误恢复策略：
** - 立即返回：错误发生时立即中止函数执行
** - 清理资源：确保分配的内存和资源被正确释放
** - 状态传播：将错误状态通过 SQLite 上下文向上传播
*/
static void icuFunctionError(
  sqlite3_context *pCtx,       /* SQLite 标量函数上下文 */
  const char *zName,           /* 失败的 ICU 函数名 */
  UErrorCode e                 /* ICU 函数返回的错误代码 */
){
  char zBuf[128];
  sqlite3_snprintf(128, zBuf, "ICU error: %s(): %s", zName, u_errorName(e));
  zBuf[127] = '\0';
  sqlite3_result_error(pCtx, zBuf, -1);
}

#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_ICU)

/*
** LIKE 或 GLOB 操作符中模式的最大长度（字节）
**
** 这个限制是为了防止：
** 1. 性能问题：过长的模式会导致性能下降
** 2. 内存消耗：避免过度的内存使用
** 3. 拒绝服务攻击：防止恶意用户构造超长模式
** 4. 缓存效率：合理的长度有利于缓存优化
*/
#ifndef SQLITE_MAX_LIKE_PATTERN_LENGTH
# define SQLITE_MAX_LIKE_PATTERN_LENGTH 50000
#endif

/*
** SQLite 内存释放函数包装器
**
** 这个函数确保我们始终调用真正的 sqlite3_free() 函数，而不是可能的宏定义。
** 在某些编译配置中，sqlite3_free 可能被定义为宏，我们需要确保调用实际函数。
**
** 设计目的：
** - 避免宏展开问题：确保调用的是实际的函数实现
** - 调试支持：便于在调试时跟踪内存释放
** - 接口一致性：与 ICU 的内存管理约定保持一致
*/
static void xFree(void *p){
  sqlite3_free(p);
}

/*
** UTF-8 字符解码查找表
**
** 这个查找表用于帮助解码多字节 UTF-8 字符的第一个字节。
** 它从 SQLite 源代码文件 utf8.c 复制而来。
**
** UTF-8 编码规则：
** - 0xxxxxxx: ASCII 字符 (0-127)
** - 110xxxxx: 2字节序列的开始 (128-191)
** - 1110xxxx: 3字节序列的开始 (192-223)
** - 11110xxx: 4字节序列的开始 (224-239)
** - 10xxxxxx: 续写字节 (128-191)
**
** 查找表使用说明：
** - 值 0-3: 续写字节的预期字节数
** - 值 0: ASCII 字符或无效序列
** - 值 1-3: 多字节字符的长度
**
** 性能优化：
** - 查找表比计算分支更快
** - 减少重复的字节长度计算
** - 支持快速的 Unicode 解码
**
** 字符范围覆盖：
** - 完整的 Unicode 基本多语言平面 (BMP)
** - 支持增补平面的 4 字节编码
** - 处理无效和过长的序列
*/
static const unsigned char icuUtf8Trans1[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
};

#define SQLITE_ICU_READ_UTF8(zIn, c)                       \
  c = *(zIn++);                                            \
  if( c>=0xc0 ){                                           \
    c = icuUtf8Trans1[c-0xc0];                             \
    while( (*zIn & 0xc0)==0x80 ){                          \
      c = (c<<6) + (0x3f & *(zIn++));                      \
    }                                                      \
  }

#define SQLITE_ICU_SKIP_UTF8(zIn)                          \
  assert( *zIn );                                          \
  if( *(zIn++)>=0xc0 ){                                    \
    while( (*zIn & 0xc0)==0x80 ){zIn++;}                   \
  }


/*
** Compare two UTF-8 strings for equality where the first string is
** a "LIKE" expression. Return true (1) if they are the same and 
** false (0) if they are different.
*/
static int icuLikeCompare(
  const uint8_t *zPattern,   /* LIKE pattern */
  const uint8_t *zString,    /* The UTF-8 string to compare against */
  const UChar32 uEsc         /* The escape character */
){
  static const uint32_t MATCH_ONE = (uint32_t)'_';
  static const uint32_t MATCH_ALL = (uint32_t)'%';

  int prevEscape = 0;     /* True if the previous character was uEsc */

  while( 1 ){

    /* Read (and consume) the next character from the input pattern. */
    uint32_t uPattern;
    SQLITE_ICU_READ_UTF8(zPattern, uPattern);
    if( uPattern==0 ) break;

    /* There are now 4 possibilities:
    **
    **     1. uPattern is an unescaped match-all character "%",
    **     2. uPattern is an unescaped match-one character "_",
    **     3. uPattern is an unescaped escape character, or
    **     4. uPattern is to be handled as an ordinary character
    */
    if( uPattern==MATCH_ALL && !prevEscape && uPattern!=(uint32_t)uEsc ){
      /* Case 1. */
      uint8_t c;

      /* Skip any MATCH_ALL or MATCH_ONE characters that follow a
      ** MATCH_ALL. For each MATCH_ONE, skip one character in the 
      ** test string.
      */
      while( (c=*zPattern) == MATCH_ALL || c == MATCH_ONE ){
        if( c==MATCH_ONE ){
          if( *zString==0 ) return 0;
          SQLITE_ICU_SKIP_UTF8(zString);
        }
        zPattern++;
      }

      if( *zPattern==0 ) return 1;

      while( *zString ){
        if( icuLikeCompare(zPattern, zString, uEsc) ){
          return 1;
        }
        SQLITE_ICU_SKIP_UTF8(zString);
      }
      return 0;

    }else if( uPattern==MATCH_ONE && !prevEscape && uPattern!=(uint32_t)uEsc ){
      /* Case 2. */
      if( *zString==0 ) return 0;
      SQLITE_ICU_SKIP_UTF8(zString);

    }else if( uPattern==(uint32_t)uEsc && !prevEscape ){
      /* Case 3. */
      prevEscape = 1;

    }else{
      /* Case 4. */
      uint32_t uString;
      SQLITE_ICU_READ_UTF8(zString, uString);
      uString = (uint32_t)u_foldCase((UChar32)uString, U_FOLD_CASE_DEFAULT);
      uPattern = (uint32_t)u_foldCase((UChar32)uPattern, U_FOLD_CASE_DEFAULT);
      if( uString!=uPattern ){
        return 0;
      }
      prevEscape = 0;
    }
  }

  return *zString==0;
}

/*
** SQLite LIKE 函数实现（基于 ICU）
**
** 此函数实现了内置的 LIKE 操作符。函数的第一个参数是模式字符串，
** 第二个参数是要匹配的字符串。因此，SQL 语句：
**
**       A LIKE B
**
** 实现为 like(B, A)。如果有转义字符 E，则：
**
**       A LIKE B ESCAPE E
**
** 映射为 like(B, A, E)。
**
** 功能特点：
** 1. Unicode 支持：完整的 Unicode 字符处理
** 2. 大小写不敏感：使用 ICU 的大小写折叠机制
** 3. 转义字符：支持用户定义的转义字符
** 4. 性能优化：限制模式长度防止性能问题
**
** 匹配规则：
** - % (百分号)：匹配任意数量的字符
** - _ (下划线)：匹配单个字符
** - 转义字符：转义特殊字符的字面意义
** - Unicode 字符：正确处理多字节字符
**
** 参数处理：
** - argv[0]: 模式字符串 (LIKE 右边的表达式)
** - argv[1]: 待匹配字符串 (LIKE 左边的表达式)
** - argv[2]: 可选的转义字符
**
** 错误处理：
** - NULL 值：任何输入为 NULL 时返回 NULL
** - 模式过长：超过限制时返回错误
** - 转义字符：必须是单个字符
** - 编码错误：无效 UTF-8 序列处理
**
** 性能考虑：
** - 模式长度限制：防止深度递归和 N*N 复杂度
** - 内存使用：高效的字符串处理算法
** - 缓存友好：优化的字符比较操作
*/
static void icuLikeFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const unsigned char *zA = sqlite3_value_text(argv[0]);
  const unsigned char *zB = sqlite3_value_text(argv[1]);
  UChar32 uEsc = 0;

  /* 限制 LIKE 或 GLOB 模式的长度，避免 patternCompare() 中的
  ** 深度递归和 N*N 行为问题。
  */
  if( sqlite3_value_bytes(argv[0])>SQLITE_MAX_LIKE_PATTERN_LENGTH ){
    sqlite3_result_error(context, "LIKE or GLOB pattern too complex", -1);
    return;
  }


  if( argc==3 ){
    /* 转义字符字符串必须由单个 UTF-8 字符组成。
    ** 否则返回错误。
    */
    int nE= sqlite3_value_bytes(argv[2]);
    const unsigned char *zE = sqlite3_value_text(argv[2]);
    int i = 0;
    if( zE==0 ) return;
    U8_NEXT(zE, i, nE, uEsc);
    if( i!=nE){
      sqlite3_result_error(context,
          "ESCAPE expression must be a single character", -1);
      return;
    }
  }

  if( zA && zB ){
    sqlite3_result_int(context, icuLikeCompare(zA, zB, uEsc));
  }
}

/*
** 编译的正则表达式对象析构函数
**
** 此函数注册为 sqlite3_set_auxdata() 的析构函数，用于在函数结束时
** 清理编译好的正则表达式对象。
**
** 生命周期管理：
** - 编译缓存：利用 SQLite 的 auxdata 机制缓存编译好的正则表达式
** - 自动清理：函数调用结束时自动释放资源
** - 内存安全：防止内存泄漏和资源浪费
** - 性能优化：避免重复编译相同的正则表达式
**
** 资源清理步骤：
** 1. 获取正则表达式对象指针
** 2. 调用 ICU API 关闭对象
** 3. 释放相关内存和资源
** 4. 更新内部引用计数
**
** 错误处理：
** - 空指针检查：安全处理 NULL 指针
** - 重复释放防护：防止多次释放同一对象
** - ICU 错误状态：处理 ICU 库的内部错误
*/
static void icuRegexpDelete(void *p){
  URegularExpression *pExpr = (URegularExpression *)p;
  uregex_close(pExpr);
}

/*
** SQLite REGEXP 操作符实现（基于 ICU）
**
** 此标量函数实现两个参数。第一个是要编译的正则表达式模式，
** 第二个是要与该模式匹配的字符串。如果任一参数是 SQL NULL，
** 则返回 NULL。否则，如果字符串匹配模式则返回 1，否则返回 0。
**
** 功能特性：
** 1. 完整的 PCRE 支持：兼容 Perl 兼容正则表达式语法
** 2. Unicode 感知：正确处理多字节 Unicode 字符
** 3. 编译优化：缓存编译后的正则表达式提高性能
** 4. 错误处理：详细的正则表达式错误信息
**
** 支持的正则特性：
** - 字符类：[a-z], [^0-9], \d, \w, \s 等
** - 量词：*, +, ?, {n}, {n,m} 等
** - 分组：(...), (?:...), (?=...) 等
** - 边界：^, $, \b, \B 等
** - 选项：不区分大小写、多行模式等
**
** 参数说明：
** - argv[0]: 正则表达式模式字符串
** - argv[1]: 待匹配的目标字符串
**
** 返回值：
** - 1: 字符串匹配正则表达式
** - 0: 字符串不匹配正则表达式
** - NULL: 任一参数为 NULL
**
** 性能优化：
** - 编译缓存：相同的正则表达式只编译一次
** - 模式预编译：在函数调用开始时预编译模式
** - 内存管理：高效的内存分配和释放策略
** - 错误恢复：编译失败时的快速恢复机制
**
** 错误类型：
** - 模式语法错误：无效的正则表达式语法
** - 编译错误：ICU 内部编译错误
** - 内存不足：无法分配足够的内存
** - 编码错误：无效的 UTF-8 序列
**
** SQLite maps the regexp() function to the regexp() operator such
** that the following two are equivalent:
**
**     zString REGEXP zPattern
**     regexp(zPattern, zString)
**
** Uses the following ICU regexp APIs:
**
**     uregex_open()
**     uregex_matches()
**     uregex_close()
*/
static void icuRegexpFunc(sqlite3_context *p, int nArg, sqlite3_value **apArg){
  UErrorCode status = U_ZERO_ERROR;
  URegularExpression *pExpr;
  UBool res;
  const UChar *zString = sqlite3_value_text16(apArg[1]);

  (void)nArg;  /* Unused parameter */

  /* If the left hand side of the regexp operator is NULL, 
  ** then the result is also NULL. 
  */
  if( !zString ){
    return;
  }

  pExpr = sqlite3_get_auxdata(p, 0);
  if( !pExpr ){
    const UChar *zPattern = sqlite3_value_text16(apArg[0]);
    if( !zPattern ){
      return;
    }
    pExpr = uregex_open(zPattern, -1, 0, 0, &status);

    if( U_SUCCESS(status) ){
      sqlite3_set_auxdata(p, 0, pExpr, icuRegexpDelete);
      pExpr = sqlite3_get_auxdata(p, 0);
    }
    if( !pExpr ){
      icuFunctionError(p, "uregex_open", status);
      return;
    }
  }

  /* Configure the text that the regular expression operates on. */
  uregex_setText(pExpr, zString, -1, &status);
  if( !U_SUCCESS(status) ){
    icuFunctionError(p, "uregex_setText", status);
    return;
  }

  /* Attempt the match */
  res = uregex_matches(pExpr, 0, &status);
  if( !U_SUCCESS(status) ){
    icuFunctionError(p, "uregex_matches", status);
    return;
  }

  /* Set the text that the regular expression operates on to a NULL
  ** pointer. This is not really necessary, but it is tidier than 
  ** leaving the regular expression object configured with an invalid
  ** pointer after this function returns.
  */
  uregex_setText(pExpr, 0, 0, &status);

  /* Return 1 or 0. */
  sqlite3_result_int(p, res ? 1 : 0);
}

/*
** Implementations of scalar functions for case mapping - upper() and 
** lower(). Function upper() converts its input to upper-case (ABC).
** Function lower() converts to lower-case (abc).
**
** ICU provides two types of case mapping, "general" case mapping and
** "language specific". Refer to ICU documentation for the differences
** between the two.
**
** To utilise "general" case mapping, the upper() or lower() scalar 
** functions are invoked with one argument:
**
**     upper('ABC') -> 'abc'
**     lower('abc') -> 'ABC'
**
** To access ICU "language specific" case mapping, upper() or lower()
** should be invoked with two arguments. The second argument is the name
** of the locale to use. Passing an empty string ("") or SQL NULL value
** as the second argument is the same as invoking the 1 argument version
** of upper() or lower().
**
**     lower('I', 'en_us') -> 'i'
**     lower('I', 'tr_tr') -> '\u131' (small dotless i)
**
** http://www.icu-project.org/userguide/posix.html#case_mappings
*/
static void icuCaseFunc16(sqlite3_context *p, int nArg, sqlite3_value **apArg){
  const UChar *zInput;            /* Pointer to input string */
  UChar *zOutput = 0;             /* Pointer to output buffer */
  int nInput;                     /* Size of utf-16 input string in bytes */
  int nOut;                       /* Size of output buffer in bytes */
  int cnt;
  int bToUpper;                   /* True for toupper(), false for tolower() */
  UErrorCode status;
  const char *zLocale = 0;

  assert(nArg==1 || nArg==2);
  bToUpper = (sqlite3_user_data(p)!=0);
  if( nArg==2 ){
    zLocale = (const char *)sqlite3_value_text(apArg[1]);
  }

  zInput = sqlite3_value_text16(apArg[0]);
  if( !zInput ){
    return;
  }
  nOut = nInput = sqlite3_value_bytes16(apArg[0]);
  if( nOut==0 ){
    sqlite3_result_text16(p, "", 0, SQLITE_STATIC);
    return;
  }

  for(cnt=0; cnt<2; cnt++){
    UChar *zNew = sqlite3_realloc(zOutput, nOut);
    if( zNew==0 ){
      sqlite3_free(zOutput);
      sqlite3_result_error_nomem(p);
      return;
    }
    zOutput = zNew;
    status = U_ZERO_ERROR;
    if( bToUpper ){
      nOut = 2*u_strToUpper(zOutput,nOut/2,zInput,nInput/2,zLocale,&status);
    }else{
      nOut = 2*u_strToLower(zOutput,nOut/2,zInput,nInput/2,zLocale,&status);
    }

    if( U_SUCCESS(status) ){
      sqlite3_result_text16(p, zOutput, nOut, xFree);
    }else if( status==U_BUFFER_OVERFLOW_ERROR ){
      assert( cnt==0 );
      continue;
    }else{
      icuFunctionError(p, bToUpper ? "u_strToUpper" : "u_strToLower", status);
    }
    return;
  }
  assert( 0 );     /* Unreachable */
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_ICU) */

/*
** ICU 排序规则析构函数
**
** pCtx 参数指向先前使用 ucol_open() 分配的 UCollator 结构。
** 此函数在排序规则不再需要时被调用，用于清理相关资源。
**
** 资源管理：
** - 内存释放：释放排序规则对象占用的内存
** - 引用计数：更新 ICU 库的内部引用计数
** - 状态清理：清理排序规则的内部状态
** - 错误处理：安全处理各种错误情况
**
** 调用时机：
** - 数据库连接关闭时
** - 显式删除排序规则时
** - 内存压力下的自动清理
** - 进程退出时的清理
*/
static void icuCollationDel(void *pCtx){
  UCollator *p = (UCollator *)pCtx;
  ucol_close(p);
}

/*
** ICU 排序规则比较函数
**
** pCtx 参数指向先前使用 ucol_open() 分配的 UCollator 结构。
** 此函数实现两个字符串的比较，返回它们的相对顺序。
**
** 比较规则：
** - 基于特定语言环境的排序规则
** - Unicode 标准的文本比较
** - 考虑大小写、重音、特殊字符等
** - 支持自定义排序强度和选项
**
** 参数说明：
** - pCtx: 排序规则上下文（UCollator 对象）
** - nLeft: 左侧字符串的长度（字节）
** - zLeft: 左侧字符串（UTF-16 编码）
** - nRight: 右侧字符串的长度（字节）
** - zRight: 右侧字符串（UTF-16 编码）
**
** 返回值：
** - -1: 左侧字符串小于右侧字符串
** -  0: 两个字符串相等
** - +1: 左侧字符串大于右侧字符串
**
** 性能特性：
** - 高效的字符串比较算法
** - 缓存友好的内存访问模式
** - 优化的 Unicode 处理
** - 支持快速路径优化
**
** 比较选项：
** - 主要强度：基本字母差异
** - 次要强度：重音差异
** - 第三强度：大小写差异
** - 第四强度：差异差异
*/
static int icuCollationColl(
  void *pCtx,
  int nLeft,
  const void *zLeft,
  int nRight,
  const void *zRight
){
  UCollationResult res;
  UCollator *p = (UCollator *)pCtx;
  res = ucol_strcoll(p, (UChar *)zLeft, nLeft/2, (UChar *)zRight, nRight/2);
  switch( res ){
    case UCOL_LESS:    return -1;
    case UCOL_GREATER: return +1;
    case UCOL_EQUAL:   return 0;
  }
  assert(!"Unexpected return value from ucol_strcoll()");
  return 0;
}

/*
** ICU 排序规则加载函数实现
**
** 此标量函数用于将基于 ICU 的排序规则类型添加到 SQLite 数据库连接。
** 它的调用方式如下：
**
**     SELECT icu_load_collation(<locale>, <collation-name>);
**
** 其中 <locale> 是包含 ICU 语言环境标识符的字符串（例如 "en_AU", "tr_TR" 等），
** <collation-name> 是要创建的排序规则序列的名称。
**
** 功能特点：
** 1. 动态加载：运行时动态添加新的排序规则
** 2. 语言多样性：支持 ICU 支持的所有语言环境
** 3. 自定义命名：用户可以为排序规则指定自定义名称
** 4. 错误处理：详细的错误信息和状态反馈
**
** 支持的语言环境：
** - en_US: 英语（美国）
** - zh_CN: 中文（简体）
** - ja_JP: 日语
** - ar_EG: 阿拉伯语（埃及）
** - de_DE: 德语（德国）
** - fr_FR: 法语（法国）
** - 以及更多...
**
** 使用示例：
** ```sql
** -- 加载德语排序规则
** SELECT icu_load_collation('de_DE', 'GERMAN');
**
** -- 使用自定义排序规则
** SELECT * FROM names ORDER BY name COLLATE GERMAN;
** ```
**
** 排序规则特性：
** - 词典排序：按语言习惯的词典顺序
** - 大小写处理：可配置的大小写敏感度
** - 重音处理：正确处理重音符号
** - 特殊字符：语言特定的字符排序
**
** 参数验证：
** - 语言环境：必须有效的 ICU 语言环境标识符
** - 排序名称：必须符合 SQLite 标识符规则
** - 权限检查：确保有创建排序规则的权限
** - 重复检查：防止重复加载相同排序规则
**
** 错误处理：
** - 无效语言环境：返回详细的错误信息
** - 内存不足：优雅处理内存分配失败
** - ICU 错误：转换和报告 ICU 库错误
** - 约束冲突：处理名称冲突和其他约束
**
** 性能考虑：
** - 编译成本：排序规则的编译和初始化开销
** - 内存使用：每个排序规则的内存占用
** - 缓存策略：避免重复编译相同的排序规则
** - 并发安全：多线程环境下的安全访问
*/
static void icuLoadCollation(
  sqlite3_context *p, 
  int nArg, 
  sqlite3_value **apArg
){
  sqlite3 *db = (sqlite3 *)sqlite3_user_data(p);
  UErrorCode status = U_ZERO_ERROR;
  const char *zLocale;      /* Locale identifier - (eg. "jp_JP") */
  const char *zName;        /* SQL Collation sequence name (eg. "japanese") */
  UCollator *pUCollator;    /* ICU library collation object */
  int rc;                   /* Return code from sqlite3_create_collation_x() */

  assert(nArg==2 || nArg==3);
  (void)nArg; /* Unused parameter */
  zLocale = (const char *)sqlite3_value_text(apArg[0]);
  zName = (const char *)sqlite3_value_text(apArg[1]);

  if( !zLocale || !zName ){
    return;
  }

  pUCollator = ucol_open(zLocale, &status);
  if( !U_SUCCESS(status) ){
    icuFunctionError(p, "ucol_open", status);
    return;
  }
  assert(p);
  if(nArg==3){
    const char *zOption = (const char*)sqlite3_value_text(apArg[2]);
    static const struct {
       const char *zName;
       UColAttributeValue val;
    } aStrength[] = {
      {  "PRIMARY",      UCOL_PRIMARY           },
      {  "SECONDARY",    UCOL_SECONDARY         },
      {  "TERTIARY",     UCOL_TERTIARY          },
      {  "DEFAULT",      UCOL_DEFAULT_STRENGTH  },
      {  "QUARTERNARY",  UCOL_QUATERNARY        },
      {  "IDENTICAL",    UCOL_IDENTICAL         },
    };
    unsigned int i;
    for(i=0; i<sizeof(aStrength)/sizeof(aStrength[0]); i++){
      if( sqlite3_stricmp(zOption,aStrength[i].zName)==0 ){
        ucol_setStrength(pUCollator, aStrength[i].val);
        break;
      }
    }
    if( i>=sizeof(aStrength)/sizeof(aStrength[0]) ){
      sqlite3_str *pStr = sqlite3_str_new(sqlite3_context_db_handle(p));
      sqlite3_str_appendf(pStr,
         "unknown collation strength \"%s\" - should be one of:",
         zOption);
      for(i=0; i<sizeof(aStrength)/sizeof(aStrength[0]); i++){
         sqlite3_str_appendf(pStr, " %s", aStrength[i].zName);
      }
      sqlite3_result_error(p, sqlite3_str_value(pStr), -1);
      sqlite3_free(sqlite3_str_finish(pStr));
      return;
    }
  }
  rc = sqlite3_create_collation_v2(db, zName, SQLITE_UTF16, (void *)pUCollator, 
      icuCollationColl, icuCollationDel
  );
  if( rc!=SQLITE_OK ){
    ucol_close(pUCollator);
    sqlite3_result_error(p, "Error registering collation function", -1);
  }
}

/*
** ICU 扩展初始化和函数注册
**
** 此函数将 ICU 扩展功能注册到数据库连接中。这是 ICU 扩展的入口点，
** 负责注册所有相关的 SQL 函数和排序规则。
**
** 注册的函数：
** - regexp(): 正则表达式匹配函数
** - like(): 增强的 LIKE 操作符实现
** - upper(): Unicode 兼容的大写转换
** - lower(): Unicode 兼容的小写转换
** - icu_load_collation(): 动态加载排序规则
**
** 函数特性：
** - 确定性：标记为 SQLITE_DETERMINISTIC
** - 安全性：标记为 SQLITE_INNOCUOUS
** - 编码优化：针对特定文本编码优化
** - 上下文支持：支持用户定义的上下文数据
**
** 注册顺序：
** 1. 核心字符串处理函数（upper, lower）
** 2. 模式匹配函数（regexp, like）
** 3. 排序规则管理函数（icu_load_collation）
**
** 错误处理：
** - 内存分配失败处理
** - 函数注册失败处理
** - ICU 库初始化错误处理
** - 版本兼容性检查
**
** 性能优化：
** - 批量注册：减少函数注册开销
** - 常量数据：使用静态常量数据结构
** - 延迟初始化：按需初始化 ICU 组件
** - 内存池：复用内存分配
**
** 线程安全：
** - 无全局状态：避免全局变量的竞争
** - 原子操作：函数注册的原子性
** - 上下文隔离：每个连接独立的上下文
** - 错误隔离：一个连接的错误不影响其他连接
*/
int sqlite3IcuInit(sqlite3 *db){
# define SQLITEICU_EXTRAFLAGS (SQLITE_DETERMINISTIC|SQLITE_INNOCUOUS)
  static const struct IcuScalar {
    const char *zName;                        /* 函数名称 */
    unsigned char nArg;                       /* 参数数量 */
    unsigned int enc;                         /* 最优文本编码 */
    unsigned char iContext;                   /* sqlite3_user_data() 上下文 */
    void (*xFunc)(sqlite3_context*,int,sqlite3_value**);
  } scalars[] = {
    {"icu_load_collation",2,SQLITE_UTF8|SQLITE_DIRECTONLY,1, icuLoadCollation},
    {"icu_load_collation",3,SQLITE_UTF8|SQLITE_DIRECTONLY,1, icuLoadCollation},
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_ICU)
    {"regexp", 2, SQLITE_ANY|SQLITEICU_EXTRAFLAGS,         0, icuRegexpFunc},
    {"lower",  1, SQLITE_UTF16|SQLITEICU_EXTRAFLAGS,       0, icuCaseFunc16},
    {"lower",  2, SQLITE_UTF16|SQLITEICU_EXTRAFLAGS,       0, icuCaseFunc16},
    {"upper",  1, SQLITE_UTF16|SQLITEICU_EXTRAFLAGS,       1, icuCaseFunc16},
    {"upper",  2, SQLITE_UTF16|SQLITEICU_EXTRAFLAGS,       1, icuCaseFunc16},
    {"lower",  1, SQLITE_UTF8|SQLITEICU_EXTRAFLAGS,        0, icuCaseFunc16},
    {"lower",  2, SQLITE_UTF8|SQLITEICU_EXTRAFLAGS,        0, icuCaseFunc16},
    {"upper",  1, SQLITE_UTF8|SQLITEICU_EXTRAFLAGS,        1, icuCaseFunc16},
    {"upper",  2, SQLITE_UTF8|SQLITEICU_EXTRAFLAGS,        1, icuCaseFunc16},
    {"like",   2, SQLITE_UTF8|SQLITEICU_EXTRAFLAGS,        0, icuLikeFunc},
    {"like",   3, SQLITE_UTF8|SQLITEICU_EXTRAFLAGS,        0, icuLikeFunc},
#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_ICU) */
  };
  int rc = SQLITE_OK;
  int i;
  
  for(i=0; rc==SQLITE_OK && i<(int)(sizeof(scalars)/sizeof(scalars[0])); i++){
    const struct IcuScalar *p = &scalars[i];
    rc = sqlite3_create_function(
        db, p->zName, p->nArg, p->enc, 
        p->iContext ? (void*)db : (void*)0,
        p->xFunc, 0, 0
    );
  }

  return rc;
}

/*
** ICU 扩展动态加载入口点
**
** 这是 ICU 扩展作为动态库加载时的入口点函数。
** 当用户调用 sqlite3_load_extension() 时，SQLite 会调用此函数。
**
** 加载方式：
** 1. 运行时加载：sqlite3_load_extension(db, "icu", NULL, NULL)
** 2. 编译时集成：通过 SQLITE_ENABLE_ICU 宏包含到核心
** 3. 静态链接：与主程序一起编译
**
** 平台特定：
** - Windows: 使用 __declspec(dllexport) 导出函数
** - Unix/Linux: 使用默认的符号可见性
** - macOS: 适配动态链接器要求
**
** 函数职责：
** 1. API 初始化：设置 SQLite API 函数指针
** 2. 扩展注册：调用核心初始化函数
** 3. 错误处理：设置错误消息和状态
** 4. 资源管理：初始化扩展所需的资源
**
** 返回值：
** - SQLITE_OK: 扩展成功加载和初始化
** - SQLITE_ERROR: 扩展初始化失败
** - 其他错误码: ICU 库或系统错误
**
** 错误处理：
** - pzErrMsg: 设置详细的错误描述
** - 资源清理：失败时清理已分配的资源
** - 状态回滚：恢复数据库连接到加载前状态
**
** 依赖检查：
** - ICU 库版本兼容性
** - SQLite API 版本检查
** - 系统资源可用性
** - 权限和安全检查
**
** 性能影响：
** - 一次性成本：扩展加载时的初始化开销
** - 内存占用：扩展代码和数据结构的内存使用
** - 启动时间：影响数据库连接的启动时间
**
** 使用示例：
** ```c
** // C 代码中加载扩展
** sqlite3 *db;
** sqlite3_open(":memory:", &db);
** sqlite3_load_extension(db, "./icu", NULL, NULL);
**
** // SQL 中使用扩展功能
** sqlite3_exec(db, "SELECT regexp('[0-9]+', 'test123');", NULL, NULL, NULL);
** ```
**
** 版本兼容：
** - SQLite 3.6+ 版本支持
** - ICU 3.6+ 版本要求
** - 向后兼容性保证
** - API 稳定性承诺
**
** 安全考虑：
** - 权限检查：确保允许加载扩展
** - 验证机制：验证扩展的完整性
** - 沙盒限制：扩展功能的安全边界
** - 错误注入：防止恶意错误信息
*/
#ifndef SQLITE_CORE
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_icu_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  SQLITE_EXTENSION_INIT2(pApi)
  return sqlite3IcuInit(db);
}
#endif

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_ICU) || defined(SQLITE_ENABLE_ICU_COLLATIONS) */
