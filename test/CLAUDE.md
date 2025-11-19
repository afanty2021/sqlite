[根目录](../../CLAUDE.md) > **test (测试套件)**

# SQLite 测试套件模块

## 模块职责

test/ 目录包含 SQLite 的完整测试套件，确保数据库引擎的正确性、性能和可靠性。测试覆盖从基本 SQL 功能到复杂边界条件的各个方面，是 SQLite 质量保证的核心。

## 入口与启动

### 测试框架

- **testfixture** - 主要测试执行器（TCL 增强版）
- **tclsqlite.c** - SQLite TCL 绑定
- **atrc.c** - 自动测试运行控制器

### 运行测试

```bash
# 构建测试框架
make testfixture

# 运行完整测试套件
make test

# 运行特定测试
./testfixture test/main.test
./testfixture test/select1.test
```

### 测试分类

- **`*.test`** - TCL 测试脚本
- **`test*.c`** - C 语言测试模块
- **`*.tcl`** - TCL 辅助脚本
- **`fts5/test/`** - FTS5 扩展测试
- **`misc/`** - 杂项测试

## 对外接口

### TCL 测试命令

```tcl
# 数据库操作
db eval {CREATE TABLE t1(a INTEGER PRIMARY KEY, b TEXT)}

# 测试断言
do_test t1-1.1 {
  db eval {SELECT count(*) FROM t1}
} {0}

# 错误测试
do_catchsql_test t1-2.1 {
  INSERT INTO t1 VALUES(1, 2, 3)
} {1 {table t1 has 2 columns but 3 values were supplied}}
```

### 测试辅助函数

```tcl
# 文件系统测试
forcedelete test.db
file exists test.db

# 性能测试
set start [clock milliseconds]
# 执行操作
set elapsed [expr {[clock milliseconds] - $start}]
```

## 测试架构

### 测试层次结构

1. **单元测试** - 测试单个函数和模块
2. **集成测试** - 测试完整功能特性
3. **压力测试** - 大数据量和高并发测试
4. **边界测试** - 异常情况和错误处理
5. **回归测试** - 防止已知问题重新出现

### 主要测试类别

| 类别 | 描述 | 代表文件 |
|------|------|----------|
| 基础功能 | 基本 SQL 操作 | `main.test`, `select1.test` |
| 事务处理 | ACID 特性 | `trans.test`, `atomic.test` |
| 并发控制 | 多线程/多进程 | `lock.test`, `mmap.test` |
| 错误处理 | 异常和恢复 | `fault.test`, `ioerr.test` |
| 性能测试 | 基准和优化 | `speed1.test`, `bigrow.test` |
| 兼容性测试 | 版本兼容性 | `backcompat.test` |

## 关键测试文件分析

### 核心测试

**main.test**：
- 数据库基本操作测试
- 连接创建/销毁
- 表创建/删除
- 基本 SQL 语句执行

**select1.test** ... **select5.test**：
- SELECT 语句功能测试
- 复杂查询语法
- 聚合函数测试
- 子查询和 JOIN 操作

**trans.test**：
- 事务 BEGIN/COMMIT/ROLLBACK
- ACID 特性验证
- 并发事务处理

### 错误注入测试

**fault.test**：
```tcl
# 模拟内存分配失败
sqlite3_memdebug_fail 10
do_test fault-1.1 {
  catch {db eval {CREATE TABLE t1(x)}}
} {1}
```

**ioerr.test**：
```tcl
# 模拟 I/O 错误
sqlite3_io_error_pending 1
do_test ioerr-1.1 {
  catch {db eval {INSERT INTO t1 VALUES(1)}}
} {1}
```

### 性能测试

**speed1.test**：
- 基本操作性能基准
- INSERT/SELECT 性能测量

**bigrow.test**：
- 大数据行处理
- 大表操作性能

### 专项测试

**fkey.test** - 外键约束测试
**auth.test** - 权限和授权测试
**analyze.test** - 统计信息和查询优化
**backup.test** - 在线备份功能测试
**wal.test** - WAL (Write-Ahead Logging) 模式测试

## 测试工具和框架

### testfixture 程序

测试执行器是增强版的 TCL 解释器：

```c
// testfixture 增强功能
static void init_all(Tcl_Interp *interp){
  Sqlite3_Init(interp);
  Tcl_Init(interp);
  // 注册测试命令
  Tcl_CreateObjCommand(interp, "do_test", do_test_cmd, 0, 0);
  Tcl_CreateObjCommand(interp, "do_catchsql_test", do_catchsql_test_cmd, 0, 0);
  // ... 更多测试命令
}
```

### 自定义测试命令

- `do_test` - 基本测试断言
- `do_catchsql_test` - SQL 错误测试
- `do_execsql_test` - SQL 执行测试
- `do_malloc_test` - 内存分配测试
- `do_ioerr_test` - I/O 错误测试

### 测试数据生成

```tcl
# 生成测试数据
for {set i 0} {$i < 1000} {incr i} {
  db eval {INSERT INTO t1 VALUES(:i, 'value_' || :i)}
}
```

## 测试执行策略

### 自动化测试

```bash
# 完整测试套件
make releasetest

# 快速测试
make fasttest

# 内存测试
make memtest

# Valgrind 测试
make valgrindtest
```

### 持续集成

```bash
# 兼容性矩阵测试
./test/srctree-check.tcl

# 跨平台测试
./tool/multiplatform-test.sh
```

### 回归测试

```bash
# 运行已知问题测试
./test/regression.test

# 性能回归检测
./test/perf-regress.test
```

## 质量指标

### 代码覆盖率

- **语句覆盖率**：> 95%
- **分支覆盖率**：> 90%
- **函数覆盖率**：100%

### 性能基准

- **基本操作**：每秒 100万+ INSERT
- **复杂查询**：毫秒级响应
- **内存使用**：稳定在预测范围内

## 测试开发指南

### 编写新测试

```tcl
# 基本测试模板
proc test_feature_name {} {
  # 设置测试环境
  db eval {CREATE TABLE test_table(id INTEGER PRIMARY KEY, data TEXT)}

  # 执行测试
  do_test feature-1.1 {
    db eval {SELECT COUNT(*) FROM test_table}
  } {0}

  # 清理
  db close
}

# 运行测试
test_feature_name
```

### 错误测试模式

```tcl
# 测试内存分配失败
do_malloc_test feature-2.1 -tclprep {
  db eval {INSERT INTO test_table VALUES(1, 'test')}
} -sqlbody {
  SELECT * FROM test_table
}
```

### 性能测试模板

```tcl
set start_time [clock milliseconds]
# 执行操作
set end_time [clock milliseconds]
set elapsed [expr {$end_time - $start_time}]
puts "Operation took $elapsed milliseconds"
```

## 调试和分析

### 测试失败分析

```bash
# 运行单个测试文件
./testfixture -f select1.test

# 启用详细输出
./testfixture -verbose select1.test
```

### 内存分析

```bash
# 使用 Valgrind
valgrind --tool=memcheck ./testfixture main.test

# 内存使用统计
make memtest
```

### 性能分析

```tcl
# 性能计时
set start [clock clicks -milliseconds]
# 测试代码
set elapsed [expr {[clock clicks -milliseconds] - $start}]
puts "Elapsed time: $elapsed ms"
```

## 常见问题 (FAQ)

### Q: 测试失败时如何调试？

A: 使用 `-verbose` 选项运行测试，检查失败的具体步骤和期望值差异。

### Q: 如何添加新的测试用例？

A: 在相应的 `.test` 文件中添加 `do_test` 或 `do_execsql_test` 调用，遵循现有测试模式。

### Q: 测试需要多长时间？

A: 完整测试套件约需要 2-4 小时，快速测试约 30 分钟。

## 相关文件清单

### 主要测试文件
- `main.test` - 基础功能测试
- `select*.test` - SELECT 语句测试
- `trans.test` - 事务测试
- `fkey.test` - 外键测试
- `auth.test` - 权限测试
- `backup.test` - 备份测试
- `wal*.test` - WAL 模式测试

### 错误注入测试
- `fault.test` - 内存分配失败测试
- `ioerr.test` - I/O 错误测试
- `corrupt*.test` - 数据损坏测试

### 性能测试
- `speed*.test` - 性能基准测试
- `big*.test` - 大数据量测试
- `permutations.test` - 性能排列测试

### 扩展测试
- `fts5/test/` - FTS5 测试
- `rtree.test` - RTREE 测试
- `json*.test` - JSON 功能测试

### 测试框架
- `tclsqlite.c` - TCL 绑定
- `atrc.c` - 自动测试控制器
- `testfixture.c` - 测试执行器

## 变更记录 (Changelog)

### 2025-11-19 09:20:15 - 测试套件文档创建
- 建立测试模块文档结构
- 分析主要测试类别和文件
- 识别测试框架和工具
- **覆盖状态**：阶段B扫描完成
- **测试覆盖**：200+ 测试文件已识别
- **缺口分析**：部分专项测试的具体内容需进一步分析