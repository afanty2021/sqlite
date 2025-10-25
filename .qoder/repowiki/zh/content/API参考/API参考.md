# API参考

<cite>
**本文档中引用的文件**  
- [sqlite3ext.h](file://src/sqlite3ext.h)
- [main.c](file://src/main.c)
- [prepare.c](file://src/prepare.c)
- [vdbeapi.c](file://src/vdbeapi.c)
- [legacy.c](file://src/legacy.c)
</cite>

## 目录
1. [简介](#简介)
2. [核心API函数](#核心api函数)
3. [错误码](#错误码)
4. [使用模式](#使用模式)
5. [内存管理](#内存管理)
6. [代码示例](#代码示例)

## 简介
SQLite提供了一个简洁而强大的C语言API，用于数据库操作。本参考文档详细介绍了主要的API函数、它们的参数、返回值以及使用模式。API设计为线程安全，并支持预编译语句以提高性能和安全性。

**Section sources**
- [main.c](file://src/main.c#L1-L50)

## 核心API函数

### sqlite3_open
打开一个SQLite数据库连接。

**原型**
```c
int sqlite3_open(const char *filename, sqlite3 **ppDb);
```

**参数**
- `filename`: 数据库文件的路径。如果文件不存在，则会创建它。
- `ppDb`: 指向数据库连接句柄的指针，成功时将被设置。

**返回值**
返回一个整数结果码。如果成功打开数据库，则返回`SQLITE_OK`；否则返回相应的错误码。

**Section sources**
- [main.c](file://src/main.c#L3661-L3750)

### sqlite3_prepare_v2
将SQL文本编译成预编译语句。

**原型**
```c
int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
```

**参数**
- `db`: 数据库连接句柄。
- `zSql`: 要编译的SQL语句。
- `nByte`: SQL语句的字节数。如果为负数，则函数会自动计算长度。
- `ppStmt`: 指向预编译语句对象的指针，成功时将被设置。
- `pzTail`: 指向剩余未处理SQL文本的指针。

**返回值**
返回一个整数结果码。如果成功编译SQL语句，则返回`SQLITE_OK`；否则返回相应的错误码。

**Section sources**
- [prepare.c](file://src/prepare.c#L923-L1000)

### sqlite3_step
执行一个预编译语句。

**原型**
```c
int sqlite3_step(sqlite3_stmt *pStmt);
```

**参数**
- `pStmt`: 要执行的预编译语句对象。

**返回值**
返回一个整数结果码。可能的返回值包括：
- `SQLITE_ROW`: 成功返回一行结果。
- `SQLITE_DONE`: 语句执行完成。
- 其他错误码表示执行失败。

**Section sources**
- [vdbeapi.c](file://src/vdbeapi.c#L894-L950)

### sqlite3_column_text
从结果集中获取指定列的文本值。

**原型**
```c
const unsigned char *sqlite3_column_text(sqlite3_stmt *pStmt, int iCol);
```

**参数**
- `pStmt`: 预编译语句对象。
- `iCol`: 列索引（从0开始）。

**返回值**
返回指向文本值的指针。如果该列的值为NULL，则返回空指针。

**Section sources**
- [vdbeapi.c](file://src/vdbeapi.c#L1358-L1422)

## 错误码

SQLite使用整数错误码来表示操作结果。以下是主要的错误码：

| 错误码 | 描述 |
|--------|------|
| SQLITE_OK | 操作成功完成 |
| SQLITE_ERROR | SQL错误或缺失的数据库 |
| SQLITE_INTERNAL | 内部逻辑错误 |
| SQLITE_PERM | 访问权限被拒绝 |
| SQLITE_ABORT | 查询执行被中断 |
| SQLITE_BUSY | 数据库文件被锁定 |
| SQLITE_LOCKED | 数据库表被锁定 |
| SQLITE_NOMEM | 内存分配失败 |
| SQLITE_READONLY | 尝试修改只读数据库 |
| SQLITE_INTERRUPT | 操作被sqlite3_interrupt()中断 |
| SQLITE_IOERR | I/O错误 |
| SQLITE_CORRUPT | 数据库磁盘镜像损坏 |
| SQLITE_NOTFOUND | 对象未找到 |
| SQLITE_FULL | 数据库或磁盘已满 |
| SQLITE_CANTOPEN | 无法打开数据库文件 |
| SQLITE_PROTOCOL | 锁定协议错误 |
| SQLITE_EMPTY | 数据库为空 |
| SQLITE_SCHEMA | 数据库模式已更改 |
| SQLITE_TOOBIG | 字符串或BLOB太长 |
| SQLITE_CONSTRAINT | 约束失败 |
| SQLITE_MISMATCH | 数据类型不匹配 |
| SQLITE_MISUSE | 不正确的库使用 |
| SQLITE_ROW | `sqlite3_step()`成功返回一行 |
| SQLITE_DONE | `sqlite3_step()`已完成执行 |

**Section sources**
- [main.c](file://src/main.c#L1541-L1608)
- [ext/wasm/api/sqlite3-wasm.c](file://ext/wasm/api/sqlite3-wasm.c#L760-L842)

## 使用模式

### 预编译语句的安全使用
预编译语句是防止SQL注入攻击的关键机制。通过将SQL语句与参数分离，可以确保用户输入不会被解释为SQL代码。

使用预编译语句的典型流程：
1. 使用`sqlite3_prepare_v2()`编译SQL语句
2. 使用`sqlite3_bind_*()`系列函数绑定参数
3. 使用`sqlite3_step()`执行语句
4. 使用`sqlite3_column_*()`系列函数获取结果
5. 使用`sqlite3_finalize()`释放预编译语句

这种方法确保了即使用户输入包含恶意SQL代码，也会被当作普通数据处理，从而有效防止SQL注入。

**Section sources**
- [prepare.c](file://src/prepare.c#L923-L1000)
- [vdbeapi.c](file://src/vdbeapi.c#L894-L950)

## 内存管理

SQLite的API在内存管理方面有明确的责任划分：

- **分配内存**: 当API函数需要返回动态分配的内存时（如`sqlite3_malloc`），调用者负责在使用完毕后调用`sqlite3_free`释放内存。
- **释放内存**: `sqlite3_free`函数用于释放由SQLite API分配的内存。
- **结果集内存**: 由`sqlite3_column_text`等函数返回的指针指向SQLite内部缓冲区，调用者不应尝试释放这些内存。这些指针在下一次调用`sqlite3_step`或`sqlite3_finalize`时失效。
- **数据库连接**: 使用`sqlite3_close`关闭数据库连接，释放相关资源。
- **预编译语句**: 使用`sqlite3_finalize`释放预编译语句占用的资源。

**Section sources**
- [main.c](file://src/main.c#L1342-L1350)
- [vdbeapi.c](file://src/vdbeapi.c#L98-L120)

## 代码示例

以下是一个完整的代码示例，展示了如何使用SQLite C API执行常见的数据库操作：

```c
#include <sqlite3.h>
#include <stdio.h>

int main() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    // 打开数据库
    rc = sqlite3_open("example.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // 创建表
    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER)", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "创建表失败: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // 准备插入语句
    const char *sql = "INSERT INTO users (name, age) VALUES (?, ?)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "准备语句失败: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // 绑定参数并执行
    sqlite3_bind_text(stmt, 1, "Alice", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, 30);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "执行插入失败: %s\n", sqlite3_errmsg(db));
    }

    // 重置语句以重用
    sqlite3_reset(stmt);

    // 插入另一行
    sqlite3_bind_text(stmt, 1, "Bob", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, 25);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "执行插入失败: %s\n", sqlite3_errmsg(db));
    }

    // 释放预编译语句
    sqlite3_finalize(stmt);

    // 查询数据
    sql = "SELECT id, name, age FROM users";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "准备查询失败: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // 遍历结果
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char*)sqlite3_column_text(stmt, 1);
        int age = sqlite3_column_int(stmt, 2);
        printf("ID: %d, Name: %s, Age: %d\n", id, name, age);
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "查询执行失败: %s\n", sqlite3_errmsg(db));
    }

    // 释放查询语句
    sqlite3_finalize(stmt);

    // 关闭数据库
    sqlite3_close(db);
    return 0;
}
```

**Section sources**
- [main.c](file://src/main.c#L3661-L3750)
- [prepare.c](file://src/prepare.c#L923-L1000)
- [vdbeapi.c](file://src/vdbeapi.c#L894-L950)
- [legacy.c](file://src/legacy.c#L29-L100)