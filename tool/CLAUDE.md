[根目录](../../CLAUDE.md) > **tool (构建工具)**

# SQLite 构建工具模块

## 模块职责

tool/ 目录包含 SQLite 构建和开发过程中使用的各种工具和脚本，包括代码生成器、解析器生成器、构建工具、测试工具等。这些工具是 SQLite 开发流程的重要组成部分。

## 入口与启动

### 主要构建工具

- **lemon.c** - Lemon LALR(1) 解析器生成器
- **mksqlite3c.tcl** - 生成 amalgamation 源文件
- **mksqlite3h.tcl** - 生成 sqlite3.h 头文件
- **mkopcodeh.tcl** - 生成操作码定义

### 使用示例

```bash
# 生成解析器
./tool/lemon sqlite/src/parse.y

# 生成 amalgamation 源文件
tclsh tool/mksqlite3c.tcl

# 生成头文件
tclsh tool/mksqlite3h.tcl
```

## 对外接口

### 代码生成工具

| 工具 | 输入 | 输出 | 用途 |
|------|------|------|------|
| `lemon` | `.y` 语法文件 | `.c` 解析器代码 | SQL 解析器生成 |
| `mksqlite3c.tcl` | 源文件列表 | `sqlite3.c` | Amalgamation 源文件 |
| `mksqlite3h.tcl` | `sqlite.h.in` | `sqlite3.h` | 公共 API 头文件 |
| `mkopcodeh.tcl` | `vdbe.c` | `opcodes.h` | 虚拟机操作码定义 |

### 诊断和分析工具

- **showdb.c** - 显示数据库内部结构
- **showjournal.c** - 显示回滚日志内容
- **showwal.c** - 显示 WAL 文件内容
- **sqldiff.c** - 比较两个数据库的差异
- **sqlite3_analyzer.c.in** - 数据库空间分析工具

### 构建脚本

- **build-all-msvc.bat** - Windows MSVC 完整构建
- **build-shell.sh** - 构建 sqlite3 命令行工具
- **emcc.sh.in** - Emscripten WebAssembly 构建

## 关键依赖与配置

### Lemon 解析器生成器

```c
// Lemon 生成的解析器结构
typedef struct yyParser yyParser;
struct yyParser {
  int yyidx;                    // 栈顶索引
  int yyerrcnt;                 // 错误计数
  YYSTACK_STATE yyStack;        // 解析栈
  struct yyParserState *yystack;  // 状态栈
};
```

### Amalgamation 构建配置

```tcl
# mksqlite3c.tcl 关键配置
set src_files [list \
  sqlite3.c \
  alter.c analyze.c attach.c \
  # ... 更多源文件
]

# 处理 include 指令
proc process_include {line} {
  # 展开 #include 内容到 amalgamation 中
}
```

## 核心工具详解

### Lemon 解析器生成器

**特性**：
- LALR(1) 解析器生成器（类似于 YACC/BISON）
- 为 SQLite 专门优化
- 生成无冲突的 SQL 解析器
- 支持错误恢复和语法错误报告

**关键文件**：
- `lemon.c` - 主要实现
- `lempar.c` - 解析器模板

**使用方法**：

```bash
# 生成 SQL 解析器
./lemon -s -q sqlite/src/parse.y
```

### Amalgamation 生成工具

**mksqlite3c.tcl**：
- 将所有源文件合并成单一的 `sqlite3.c`
- 解决内部 #include 依赖关系
- 优化编译器内联和优化效果

**mksqlite3h.tcl**：
- 从 `sqlite.h.in` 生成最终的 `sqlite3.h`
- 插入版本号和源码标识
- 处理条件编译指令

### 代码生成脚本

| 脚本 | 功能 | 输入 | 输出 |
|------|------|------|------|
| `mkkeywordhash.c` | 生成 SQL 关键字哈希表 | - | `keywordhash.h` |
| `mkpragmatab.tcl` | 生成 PRAGMA 处理代码 | 配置表 | `pragma.h` |
| `mkopcodec.tcl` | 生成操作码名称映射 | `opcodes.h` | `opcodes.c` |

### 数据库诊断工具

**showdb.c**：
```bash
# 显示数据库页面结构
./showdb database.db
```

**sqlite3_analyzer**：
```bash
# 分析数据库空间使用情况
./sqlite3_analyzer database.db
```

**sqldiff.c**：
```bash
# 比较两个数据库
./sqldiff db1.db db2.db
```

## 测试与质量

### 构建验证

```bash
# 验证源码完整性
make verify-source

# 构建所有工具
make tool-build
```

### 工具测试

```bash
# 测试 Lemon 解析器
./test_grammar

# 测试 Amalgamation 生成
./test_amalgamation
```

### 质量保证

- **内存安全**：所有工具都通过内存检查
- **跨平台**：支持 Unix、Windows、macOS
- **向后兼容**：保持工具接口稳定

## 构建流程

### 完整构建流程

```bash
# 1. 生成解析器
make parse.c

# 2. 生成头文件
make sqlite3.h

# 3. 生成 amalgamation
make sqlite3.c

# 4. 构建核心库
make sqlite3.a

# 5. 构建命令行工具
make sqlite3

# 6. 运行测试
make test
```

### 自动化脚本

**mkvsix.tcl** - Visual Studio 扩展打包：

```tcl
# 生成 VSIX 扩展包
tclsh tool/mkvsix.tcl
```

**warnings.sh** - 编译警告检查：

```bash
# 检查编译警告
./tool/warnings.sh
```

## 开发指南

### 添加新工具

1. **创建 C 源文件**：

```c
#include "sqlite3.h"

int main(int argc, char **argv){
  sqlite3 *db;
  int rc = sqlite3_open(argv[1], &db);
  // 工具逻辑实现
  sqlite3_close(db);
  return 0;
}
```

2. **添加到 Makefile**：

```makefile
mynewtool: mynewtool.c
	$(CC) $(OPTS) mynewtool.c -o mynewtool
```

### 自定义构建

**修改 mksqlite3c.tcl**：

```tcl
# 添加自定义源文件
lappend src_files "mycustom.c"
```

**扩展 mkpragmatab.tcl**：

```tcl
# 添加新 PRAGMA
set pragma_list {
  # 现有 PRAGMA...
  {my_pragma 1 "Description of my pragma"}
}
```

## 常见问题 (FAQ)

### Q: Lemon 和 YACC 有什么区别？

A: Lemon 是专门为 SQLite 开发的解析器生成器，具有更好的错误恢复机制和更简洁的接口。

### Q: Amalgamation 有什么优势？

A: 将所有源文件合并为单一文件，允许编译器进行更好的跨函数优化，提高性能约 5%。

### Q: 如何添加新的 SQL 函数？

A: 在 `func.c` 中添加函数实现，或创建扩展模块。使用标准 API 注册函数。

## 相关文件清单

### 解析器生成器
- `lemon.c` - Lemon 解析器生成器
- `lempar.c` - Lemon 模板文件

### 代码生成工具
- `mksqlite3c.tcl` - Amalgamation 生成器
- `mksqlite3h.tcl` - 头文件生成器
- `mkopcodeh.tcl` - 操作码定义生成器
- `mkopcodec.tcl` - 操作码映射生成器
- `mkkeywordhash.c` - 关键字哈希生成器
- `mkpragmatab.tcl` - PRAGMA 表生成器

### 数据库工具
- `showdb.c` - 数据库结构显示工具
- `showjournal.c` - 日志文件显示工具
- `showwal.c` - WAL 文件显示工具
- `sqldiff.c` - 数据库比较工具
- `sqlite3_analyzer.c.in` - 空间分析工具
- `dbhash.c` - 数据库哈希工具

### 构建脚本
- `build-all-msvc.bat` - Windows 构建脚本
- `build-shell.sh` - Shell 构建脚本
- `emcc.sh.in` - WebAssembly 构建脚本
- `warnings.sh` - 编译警告检查脚本

### 辅助工具
- `mkvsix.tcl` - Visual Studio 扩展打包
- `split-sqlite3c.tcl` - Amalgamation 分割工具
- `src-verify.c` - 源码验证工具

## 变更记录 (Changelog)

### 2025-11-19 09:20:15 - 构建工具模块文档创建
- 建立工具模块文档结构
- 分析核心工具：Lemon、Amalgamation 生成器、诊断工具
- 识别构建脚本和辅助工具
- **覆盖状态**：阶段B扫描完成
- **工具覆盖**：15+ 主要构建工具已识别
- **缺口分析**：部分小工具的具体功能需进一步分析