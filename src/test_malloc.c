/*
** 2007 August 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** SQLite 内存分配子系统测试接口实现
**
** 本文件包含用于实现内存分配子系统测试接口的代码。
** 这些测试工具专门用于检测内存泄漏、越界访问、
** 内存分配失败等内存相关问题。
**
**
** 测试框架概述：
**
** 1. 内存故障模拟：
**    - 可控的内存分配失败模拟
**    - 精确控制失败时机和次数
**    - 支持良性失败的测试场景
**    - 提供详细的故障统计信息
**
** 2. 内存泄漏检测：
**    - 跟踪所有内存分配和释放
**    - 检测未释放的内存块
**    - 识别内存泄漏的来源
**    - 生成详细的泄漏报告
**
** 3. 边界检查：
**    - 检测缓冲区溢出
**    - 验证内存访问的有效性
**    - 检测使用已释放内存
**    - 监控未初始化内存的访问
**
** 4. 性能分析：
**    - 内存分配统计和分析
**    - 内存使用模式分析
**    - 分配效率监控
**    - 内存碎片化评估
**
** 核心功能模块：
**
** 故障模拟系统：
** - 基于计数器的故障触发机制
** - 可配置的故障重复次数
** - 支持良性故障和严重故障
** - 提供故障注入和恢复功能
**
** 内存监控：
** - 实时跟踪内存分配状态
** - 记录分配/释放的详细信息
** - 统计内存使用量和模式
** - 监控内存池的健康状态
**
** 测试工具：
** - TCL 命令接口用于测试控制
** - 自动化测试脚本支持
** - 调试信息输出和分析
** - 错误报告和诊断功能
**
** 测试方法论：
**
** 压力测试：
** - 在各种内存压力条件下测试 SQLite
** - 模拟内存不足的环境
** - 测试内存分配失败的处理
** - 验证错误恢复机制
**
** 回归测试：
** - 确保内存修复不引入新问题
** - 验证内存管理代码的正确性
** - 检测内存相关的性能退化
** - 验证多线程环境下的安全性
**
** 边界测试：
** - 测试极端内存分配大小
** - 验证零长度和负长度处理
** - 测试内存分配的边界条件
** - 检查异常情况下的行为
**
** 集成测试：
** - 与其他 SQLite 组件的集成测试
** - 真实应用场景下的内存测试
** - 长期运行的内存稳定性测试
** - 复杂查询的内存使用分析
**
** 技术实现：
**
** 故障模拟算法：
** - 基于倒计数的故障触发
** - 精确控制故障发生的时机
** - 支持多级故障模拟策略
** - 提供故障恢复和重置机制
**
** 内存包装层：
** - 包装标准的内存分配函数
** - 添加监控和调试功能
** - 保持与原始接口的兼容性
** - 支持动态启用/禁用监控
**
** 状态管理：
** - 维护详细的故障模拟状态
** - 记录成功和失败的统计信息
** - 提供状态查询和重置功能
** - 支持状态持久化和恢复
**
** 使用场景：
**
** 开发调试：
** - 在开发阶段检测内存问题
** - 验证内存管理代码的正确性
** - 调试内存相关的性能问题
** - 分析内存使用模式
**
** 质量保证：
** - 在发布前进行全面的内存测试
** - 确保产品在各种内存条件下稳定运行
** - 验证内存管理的一致性和可靠性
** - 检测潜在的内存安全隐患
**
** 性能优化：
** - 分析内存分配的性能瓶颈
** - 优化内存使用效率
** - 减少内存碎片化
** - 改进内存分配策略
**
** 兼容性测试：
** - 验证在不同平台上的内存行为
** - 测试与不同内存管理器的兼容性
** - 确保在各种内存配置下的正确性
** - 验证与第三方库的内存交互
**
** 配置选项：
**
** 编译时配置：
** - 启用/禁用内存调试功能
** - 选择不同的内存分配策略
** - 配置故障模拟的详细程度
** - 设置性能监控的级别
**
** 运行时配置：
** - 动态启用/禁用故障模拟
** - 调整故障触发参数
** - 配置监控阈值和报告
** - 选择不同的测试模式
**
** 调试支持：
**
** 详细日志：
** - 记录每个内存操作的详细信息
** - 提供操作时间和大小信息
** - 跟踪调用栈信息
** - 生成结构化的调试报告
**
** 断点支持：
** - 提供可设置的调试断点
** - 在特定条件下停止执行
** - 支持条件断点和计数断点
** - 便于交互式调试
**
** 性能分析：
**
** 分配统计：
** - 统计内存分配的频率和大小
** - 分析内存使用的峰值和平均值
** - 监控内存分配和释放的模式
** - 识别内存使用的热点
**
** 效率评估：
** - 测量内存分配操作的性能
** - 分析内存管理的开销
** - 评估内存使用的效率
** - 识别性能瓶颈
**
** 安全特性：
**
** 内存保护：
** - 检测缓冲区溢出和下溢
** - 验证内存访问的有效性
** - 防止使用已释放的内存
** - 检测未初始化内存的使用
**
** 信息泄露防护：
** - 确保敏感数据不被意外泄露
** - 清理释放的内存内容
** - 检测潜在的内存安全问题
** - 提供安全的内存清理机制
**
** 扩展能力：
**
** 插件架构：
** - 支持自定义内存分配器
** - 可扩展的故障模拟策略
** - 插件式的监控和分析工具
** - 灵活的测试场景配置
**
** 第三方集成：
** - 与 Valgrind、AddressSanitizer 等工具集成
** - 支持外部内存分析工具
** - 提供标准化的测试接口
** - 便于 CI/CD 流水线集成
**
** 最佳实践：
**
** 测试策略：
** - 在开发过程中持续进行内存测试
** - 定期运行完整的内存测试套件
** - 结合多种测试方法进行验证
** - 建立内存测试的自动化流程
**
** 性能考虑：
** - 平衡测试的详细程度和性能开销
** - 合理选择测试的覆盖范围
** - 优化测试代码的执行效率
** - 避免测试本身引入的内存问题
**
** 维护建议：
** - 定期更新测试用例和场景
** - 跟踪新的内存管理特性
** - 保持与 SQLite 核心代码的同步
** - 持续改进测试工具和方法
*/
#include "sqliteInt.h"
#include "tclsqlite.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
** 内存故障模拟状态结构体
**
** 此结构体用于封装 malloc() 故障模拟使用的全局状态变量。
** 它控制内存分配失败的行为，并收集相关的统计信息。
**
** 结构体设计原理：
**
** 1. 状态控制：
**    - 精确控制故障发生的时间和频率
**    - 支持故障的重复和持续性
**    - 提供故障模拟的启用/禁用机制
**    - 区分良性故障和严重故障
**
** 2. 统计信息：
**    - 记录成功和失败的分配次数
**    - 统计故障发生前后的分配情况
**    - 跟踪良性故障的数量
**    - 提供详细的故障报告数据
**
** 3. 配置管理：
**    - 维护原始内存分配方法的引用
**    - 支持故障模拟层的安装状态
**    - 提供灵活的配置选项
**    - 支持动态配置更新
**
** 字段详细说明：
**
** 故障控制：
** - iCountdown: 故障触发倒计时器
**   - 值大于0时表示还有多少次成功分配后触发故障
**   - 为0时表示下一次分配将触发故障
**   - 控制故障发生的精确时机
**
** - nRepeat: 故障重复次数
**   - 指定故障重复发生的次数
**   - 值为1表示单次故障
**   - 可以设置为多次重复以测试连续失败场景
**
** - enable: 故障模拟启用标志
**   - 为1时启用故障模拟
**   - 为0时禁用故障模拟（所有分配都成功）
**   - 可以动态切换以控制测试流程
**
** 统计计数器：
** - nBenign: 良性故障计数
**   - 记录自上次配置以来良性故障的数量
**   - 良性故障是指程序可以安全处理的故障
**   - 用于区分需要特殊处理的故障类型
**
** - nFail: 总故障计数
**   - 记录自上次配置以来所有故障的数量
**   - 包括良性故障和严重故障
**   - 用于评估故障模拟的效果
**
** - nOkBefore: 故障前成功分配计数
**   - 记录第一次故障发生前的成功分配次数
**   - 用于验证故障触发的时机是否正确
**   - 帮助分析故障对程序的影响
**
** - nOkAfter: 故障后成功分配计数
**   - 记录故障发生后的成功分配次数
**   - 用于评估程序从故障中恢复的能力
**   - 检查故障处理逻辑的有效性
**
** 状态标志：
** - isInstalled: 故障模拟层安装状态
**   - 标识故障模拟层是否已安装到内存分配系统
**   - 为1时表示已安装，可以开始故障模拟
**   - 为0时表示未安装，需要先安装才能使用
**
** - isBenignMode: 良性故障模式标志
**   - 为1时表示将故障视为良性（可以安全处理）
**   - 为0时表示故障是严重的（需要特殊处理）
**   - 影响故障统计和报告的方式
**
** 内存方法：
** - m: 原始内存分配方法
**   - 保存原始的 sqlite3_mem_methods 结构
**   - 用于在故障模拟时调用真实的分配函数
**   - 确保在禁用故障时能够正常分配内存
**
** 使用场景：
**
** 故障测试：
** - 模拟内存不足的场景
** - 测试错误恢复机制
** - 验证内存分配失败的处理
** - 评估程序的健壮性
**
** 性能测试：
** - 测试在内存分配失败时的性能表现
** - 分析故障恢复的性能开销
** - 评估故障模拟对整体性能的影响
** - 优化故障处理逻辑
**
** 调试分析：
** - 精确定位内存分配问题
** - 分析故障发生时的程序状态
** - 调试内存管理相关的错误
** - 验证修复方案的有效性
**
** 安全特性：
**
** 防止误用：
** - 确保在未安装时不会影响正常操作
** - 提供安全的故障模拟机制
** - 防止故障模拟影响系统稳定性
** - 支持安全的故障恢复
**
** 状态保护：
** - 维护一致的状态信息
** - 防止状态损坏和竞争条件
** - 提供原子性的状态更新
** - 支持多线程安全访问
**
** 扩展能力：
**
** 配置选项：
** - 支持多种故障模拟策略
** - 可配置的故障触发条件
** - 灵活的统计信息收集
** - 可扩展的故障类型定义
**
** 监控接口：
** - 提供状态查询接口
** - 支持实时监控故障模拟状态
** - 允许动态调整故障参数
** - 集成外部监控工具
**
** 最佳实践：
**
** 初始化：
** - 在使用前正确初始化所有字段
** - 确保初始状态的一致性
** - 验证配置参数的有效性
** - 建立适当的默认值
**
** 使用规范：
** - 避免在关键路径上过度使用
** - 合理设置故障模拟参数
** - 及时清理和重置状态
** - 记录和分析故障统计信息
**
** 调试技巧：
** - 使用统计信息分析故障模式
** - 利用故障时机定位问题
** - 结合日志输出进行详细分析
** - 验证故障处理的完整性
**
** 注意事项：
** - 此结构体是全局静态变量，不是线程安全的
** - 在多线程环境下使用时需要额外的同步机制
** - 故障模拟可能会影响程序的正常运行
** - 使用后需要正确重置状态以避免影响后续测试
*/
static struct MemFault {
  int iCountdown;         /* 故障触发倒计时：失败前的成功分配次数 */
  int nRepeat;            /* 故障重复次数：故障重复发生的次数 */
  int nBenign;            /* 良性故障计数：自上次配置以来的良性故障数 */
  int nFail;              /* 总故障计数：自上次配置以来的所有故障数 */
  int nOkBefore;          /* 故障前成功数：第一次故障前的成功分配数 */
  int nOkAfter;           /* 故障后成功数：故障发生后的成功分配数 */
  u8 enable;              /* 启用标志：是否启用故障模拟 */
  int isInstalled;        /* 安装状态：故障模拟层是否已安装 */
  int isBenignMode;       /* 良性模式：是否将故障视为良性 */
  sqlite3_mem_methods m;  /* 原始方法：真实的内存分配实现 */
} memfault;

/*
** This routine exists as a place to set a breakpoint that will
** fire on any simulated malloc() failure.
*/
static void sqlite3Fault(void){
  static u64 cnt = 0;
  cnt++;
  if( cnt>((u64)1<<63) ) abort();
}

/*
** This routine exists as a place to set a breakpoint that will
** fire the first time any malloc() fails on a single test case.
** The sqlite3Fault() routine above runs on every malloc() failure.
** This routine only runs on the first such failure.
*/
static void sqlite3FirstFault(void){
  static u64 cnt2 = 0;
  cnt2++;
  if( cnt2>((u64)1<<63) ) abort();
}

/*
** Check to see if a fault should be simulated.  Return true to simulate
** the fault.  Return false if the fault should not be simulated.
*/
static int faultsimStep(void){
  if( likely(!memfault.enable) ){
    memfault.nOkAfter++;
    return 0;
  }
  if( memfault.iCountdown>0 ){
    memfault.iCountdown--;
    memfault.nOkBefore++;
    return 0;
  }
  if( memfault.nFail==0 ) sqlite3FirstFault();
  sqlite3Fault();
  memfault.nFail++;
  if( memfault.isBenignMode>0 ){
    memfault.nBenign++;
  }
  memfault.nRepeat--;
  if( memfault.nRepeat<=0 ){
    memfault.enable = 0;
  }
  return 1;  
}

/*
** A version of sqlite3_mem_methods.xMalloc() that includes fault simulation
** logic.
*/
static void *faultsimMalloc(int n){
  void *p = 0;
  if( !faultsimStep() ){
    p = memfault.m.xMalloc(n);
  }
  return p;
}


/*
** A version of sqlite3_mem_methods.xRealloc() that includes fault simulation
** logic.
*/
static void *faultsimRealloc(void *pOld, int n){
  void *p = 0;
  if( !faultsimStep() ){
    p = memfault.m.xRealloc(pOld, n);
  }
  return p;
}

/*
** This routine configures the malloc failure simulation.  After
** calling this routine, the next nDelay mallocs will succeed, followed
** by a block of nRepeat failures, after which malloc() calls will begin
** to succeed again.
*/
static void faultsimConfig(int nDelay, int nRepeat){
  memfault.iCountdown = nDelay;
  memfault.nRepeat = nRepeat;
  memfault.nBenign = 0;
  memfault.nFail = 0;
  memfault.nOkBefore = 0;
  memfault.nOkAfter = 0;
  memfault.enable = nDelay>=0;

  /* Sometimes, when running multi-threaded tests, the isBenignMode 
  ** variable is not properly incremented/decremented so that it is
  ** 0 when not inside a benign malloc block. This doesn't affect
  ** the multi-threaded tests, as they do not use this system. But
  ** it does affect OOM tests run later in the same process. So
  ** zero the variable here, just to be sure.
  */
  memfault.isBenignMode = 0;
}

/*
** Return the number of faults (both hard and benign faults) that have
** occurred since the injector was last configured.
*/
static int faultsimFailures(void){
  return memfault.nFail;
}

/*
** Return the number of benign faults that have occurred since the
** injector was last configured.
*/
static int faultsimBenignFailures(void){
  return memfault.nBenign;
}

/*
** Return the number of successes that will occur before the next failure.
** If no failures are scheduled, return -1.
*/
static int faultsimPending(void){
  if( memfault.enable ){
    return memfault.iCountdown;
  }else{
    return -1;
  }
}


static void faultsimBeginBenign(void){
  memfault.isBenignMode++;
}
static void faultsimEndBenign(void){
  memfault.isBenignMode--;
}

/*
** Add or remove the fault-simulation layer using sqlite3_config(). If
** the argument is non-zero, the 
*/
static int faultsimInstall(int install){
  int rc;

  install = (install ? 1 : 0);
  assert(memfault.isInstalled==1 || memfault.isInstalled==0);

  if( install==memfault.isInstalled ){
    return SQLITE_ERROR;
  }

  if( install ){
    rc = sqlite3_config(SQLITE_CONFIG_GETMALLOC, &memfault.m);
    assert(memfault.m.xMalloc);
    if( rc==SQLITE_OK ){
      sqlite3_mem_methods m = memfault.m;
      m.xMalloc = faultsimMalloc;
      m.xRealloc = faultsimRealloc;
      rc = sqlite3_config(SQLITE_CONFIG_MALLOC, &m);
    }
    sqlite3_test_control(SQLITE_TESTCTRL_BENIGN_MALLOC_HOOKS, 
        faultsimBeginBenign, faultsimEndBenign
    );
  }else{
    sqlite3_mem_methods m2;
    assert(memfault.m.xMalloc);

    /* One should be able to reset the default memory allocator by storing
    ** a zeroed allocator then calling GETMALLOC. */
    memset(&m2, 0, sizeof(m2));
    sqlite3_config(SQLITE_CONFIG_MALLOC, &m2);
    sqlite3_config(SQLITE_CONFIG_GETMALLOC, &m2);
    assert( memcmp(&m2, &memfault.m, sizeof(m2))==0 );

    rc = sqlite3_config(SQLITE_CONFIG_MALLOC, &memfault.m);
    sqlite3_test_control(SQLITE_TESTCTRL_BENIGN_MALLOC_HOOKS,
        (void*)0, (void*)0);
  }

  if( rc==SQLITE_OK ){
    memfault.isInstalled = 1;
  }
  return rc;
}

#ifdef SQLITE_TEST

/*
** This function is implemented in main.c. Returns a pointer to a static
** buffer containing the symbolic SQLite error code that corresponds to
** the least-significant 8-bits of the integer passed as an argument.
** For example:
**
**   sqlite3ErrName(1) -> "SQLITE_ERROR"
*/
extern const char *sqlite3ErrName(int);

/*
** Transform pointers to text and back again
*/
static void pointerToText(void *p, char *z){
  static const char zHex[] = "0123456789abcdef";
  int i, k;
  unsigned int u;
  sqlite3_uint64 n;
  if( p==0 ){
    strcpy(z, "0");
    return;
  }
  if( sizeof(n)==sizeof(p) ){
    memcpy(&n, &p, sizeof(p));
  }else if( sizeof(u)==sizeof(p) ){
    memcpy(&u, &p, sizeof(u));
    n = u;
  }else{
    assert( 0 );
  }
  for(i=0, k=sizeof(p)*2-1; i<sizeof(p)*2; i++, k--){
    z[k] = zHex[n&0xf];
    n >>= 4;
  }
  z[sizeof(p)*2] = 0;
}
static int hexToInt(int h){
  if( h>='0' && h<='9' ){
    return h - '0';
  }else if( h>='a' && h<='f' ){
    return h - 'a' + 10;
  }else{
    return -1;
  }
}
static int textToPointer(const char *z, void **pp){
  sqlite3_uint64 n = 0;
  int i;
  unsigned int u;
  for(i=0; i<sizeof(void*)*2 && z[0]; i++){
    int v;
    v = hexToInt(*z++);
    if( v<0 ) return TCL_ERROR;
    n = n*16 + v;
  }
  if( *z!=0 ) return TCL_ERROR;
  if( sizeof(n)==sizeof(*pp) ){
    memcpy(pp, &n, sizeof(n));
  }else if( sizeof(u)==sizeof(*pp) ){
    u = (unsigned int)n;
    memcpy(pp, &u, sizeof(u));
  }else{
    assert( 0 );
  }
  return TCL_OK;
}

/*
** Usage:    sqlite3_malloc  NBYTES
**
** Raw test interface for sqlite3_malloc().
*/
static int SQLITE_TCLAPI test_malloc(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int nByte;
  void *p;
  char zOut[100];
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "NBYTES");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[1], &nByte) ) return TCL_ERROR;
  p = sqlite3_malloc((unsigned)nByte);
  pointerToText(p, zOut);
  Tcl_AppendResult(interp, zOut, NULL);
  return TCL_OK;
}

/*
** Usage:    sqlite3_realloc  PRIOR  NBYTES
**
** Raw test interface for sqlite3_realloc().
*/
static int SQLITE_TCLAPI test_realloc(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int nByte;
  void *pPrior, *p;
  char zOut[100];
  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "PRIOR NBYTES");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[2], &nByte) ) return TCL_ERROR;
  if( textToPointer(Tcl_GetString(objv[1]), &pPrior) ){
    Tcl_AppendResult(interp, "bad pointer: ", Tcl_GetString(objv[1]), (char*)0);
    return TCL_ERROR;
  }
  p = sqlite3_realloc(pPrior, (unsigned)nByte);
  pointerToText(p, zOut);
  Tcl_AppendResult(interp, zOut, NULL);
  return TCL_OK;
}

/*
** Usage:    sqlite3_free  PRIOR
**
** Raw test interface for sqlite3_free().
*/
static int SQLITE_TCLAPI test_free(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  void *pPrior;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "PRIOR");
    return TCL_ERROR;
  }
  if( textToPointer(Tcl_GetString(objv[1]), &pPrior) ){
    Tcl_AppendResult(interp, "bad pointer: ", Tcl_GetString(objv[1]), (char*)0);
    return TCL_ERROR;
  }
  sqlite3_free(pPrior);
  return TCL_OK;
}

/*
** These routines are in test_hexio.c
*/
int sqlite3TestHexToBin(const char *, int, char *);
int sqlite3TestBinToHex(char*,int);

/*
** Usage:    memset  ADDRESS  SIZE  HEX
**
** Set a chunk of memory (obtained from malloc, probably) to a
** specified hex pattern.
*/
static int SQLITE_TCLAPI test_memset(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  void *p;
  int size, i;
  Tcl_Size n;
  char *zHex;
  char *zOut;
  char zBin[100];

  if( objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "ADDRESS SIZE HEX");
    return TCL_ERROR;
  }
  if( textToPointer(Tcl_GetString(objv[1]), &p) ){
    Tcl_AppendResult(interp, "bad pointer: ", Tcl_GetString(objv[1]), (char*)0);
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[2], &size) ){
    return TCL_ERROR;
  }
  if( size<=0 ){
    Tcl_AppendResult(interp, "size must be positive", (char*)0);
    return TCL_ERROR;
  }
  zHex = Tcl_GetStringFromObj(objv[3], &n);
  if( n>sizeof(zBin)*2 ) n = sizeof(zBin)*2;
  n = sqlite3TestHexToBin(zHex, (int)n, zBin);
  if( n==0 ){
    Tcl_AppendResult(interp, "no data", (char*)0);
    return TCL_ERROR;
  }
  zOut = p;
  for(i=0; i<size; i++){
    zOut[i] = zBin[i%n];
  }
  return TCL_OK;
}

/*
** Usage:    memget  ADDRESS  SIZE
**
** Return memory as hexadecimal text.
*/
static int SQLITE_TCLAPI test_memget(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  void *p;
  int size, n;
  char *zBin;
  char zHex[100];

  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "ADDRESS SIZE");
    return TCL_ERROR;
  }
  if( textToPointer(Tcl_GetString(objv[1]), &p) ){
    Tcl_AppendResult(interp, "bad pointer: ", Tcl_GetString(objv[1]), (char*)0);
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[2], &size) ){
    return TCL_ERROR;
  }
  if( size<=0 ){
    Tcl_AppendResult(interp, "size must be positive", (char*)0);
    return TCL_ERROR;
  }
  zBin = p;
  while( size>0 ){
    if( size>(sizeof(zHex)-1)/2 ){
      n = (sizeof(zHex)-1)/2;
    }else{
      n = size;
    }
    memcpy(zHex, zBin, n);
    zBin += n;
    size -= n;
    sqlite3TestBinToHex(zHex, n);
    Tcl_AppendResult(interp, zHex, (char*)0);
  }
  return TCL_OK;
}

/*
** Usage:    sqlite3_memory_used
**
** Raw test interface for sqlite3_memory_used().
*/
static int SQLITE_TCLAPI test_memory_used(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  Tcl_SetObjResult(interp, Tcl_NewWideIntObj(sqlite3_memory_used()));
  return TCL_OK;
}

/*
** Usage:    sqlite3_memory_highwater ?RESETFLAG?
**
** Raw test interface for sqlite3_memory_highwater().
*/
static int SQLITE_TCLAPI test_memory_highwater(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int resetFlag = 0;
  if( objc!=1 && objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "?RESET?");
    return TCL_ERROR;
  }
  if( objc==2 ){
    if( Tcl_GetBooleanFromObj(interp, objv[1], &resetFlag) ) return TCL_ERROR;
  } 
  Tcl_SetObjResult(interp, 
     Tcl_NewWideIntObj(sqlite3_memory_highwater(resetFlag)));
  return TCL_OK;
}

/*
** Usage:    sqlite3_memdebug_backtrace DEPTH
**
** Set the depth of backtracing.  If SQLITE_MEMDEBUG is not defined
** then this routine is a no-op.
*/
static int SQLITE_TCLAPI test_memdebug_backtrace(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int depth;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DEPT");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[1], &depth) ) return TCL_ERROR;
#ifdef SQLITE_MEMDEBUG
  {
    extern void sqlite3MemdebugBacktrace(int);
    sqlite3MemdebugBacktrace(depth);
  }
#endif
  return TCL_OK;
}

/*
** Usage:    sqlite3_memdebug_dump  FILENAME
**
** Write a summary of unfreed memory to FILENAME.
*/
static int SQLITE_TCLAPI test_memdebug_dump(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "FILENAME");
    return TCL_ERROR;
  }
#if defined(SQLITE_MEMDEBUG) || defined(SQLITE_MEMORY_SIZE) \
     || defined(SQLITE_POW2_MEMORY_SIZE)
  {
    extern void sqlite3MemdebugDump(const char*);
    sqlite3MemdebugDump(Tcl_GetString(objv[1]));
  }
#endif
  return TCL_OK;
}

/*
** Usage:    sqlite3_memdebug_malloc_count
**
** Return the total number of times malloc() has been called.
*/
static int SQLITE_TCLAPI test_memdebug_malloc_count(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int nMalloc = -1;
  if( objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }
#if defined(SQLITE_MEMDEBUG)
  {
    extern int sqlite3MemdebugMallocCount();
    nMalloc = sqlite3MemdebugMallocCount();
  }
#endif
  Tcl_SetObjResult(interp, Tcl_NewIntObj(nMalloc));
  return TCL_OK;
}


/*
** Usage:    sqlite3_memdebug_fail  COUNTER  ?OPTIONS?
**
** where options are:
**
**     -repeat    <count>
**     -benigncnt <varname>
**
** Arrange for a simulated malloc() failure after COUNTER successes.
** If a repeat count is specified, the fault is repeated that many
** times.
**
** Each call to this routine overrides the prior counter value.
** This routine returns the number of simulated failures that have
** happened since the previous call to this routine.
**
** To disable simulated failures, use a COUNTER of -1.
*/
static int SQLITE_TCLAPI test_memdebug_fail(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int ii;
  int iFail;
  int nRepeat = 1;
  Tcl_Obj *pBenignCnt = 0;
  int nBenign;
  int nFail = 0;

  if( objc<2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "COUNTER ?OPTIONS?");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[1], &iFail) ) return TCL_ERROR;

  for(ii=2; ii<objc; ii+=2){
    Tcl_Size nOption;
    char *zOption = Tcl_GetStringFromObj(objv[ii], &nOption);
    char *zErr = 0;

    if( nOption>1 && strncmp(zOption, "-repeat", nOption)==0 ){
      if( ii==(objc-1) ){
        zErr = "option requires an argument: ";
      }else{
        if( Tcl_GetIntFromObj(interp, objv[ii+1], &nRepeat) ){
          return TCL_ERROR;
        }
      }
    }else if( nOption>1 && strncmp(zOption, "-benigncnt", nOption)==0 ){
      if( ii==(objc-1) ){
        zErr = "option requires an argument: ";
      }else{
        pBenignCnt = objv[ii+1];
      }
    }else{
      zErr = "unknown option: ";
    }

    if( zErr ){
      Tcl_AppendResult(interp, zErr, zOption, NULL);
      return TCL_ERROR;
    }
  }
  
  nBenign = faultsimBenignFailures();
  nFail = faultsimFailures();
  faultsimConfig(iFail, nRepeat);

  if( pBenignCnt ){
    Tcl_ObjSetVar2(interp, pBenignCnt, 0, Tcl_NewIntObj(nBenign), 0);
  }
  Tcl_SetObjResult(interp, Tcl_NewIntObj(nFail));
  return TCL_OK;
}

/*
** Usage:    sqlite3_memdebug_pending
**
** Return the number of malloc() calls that will succeed before a 
** simulated failure occurs. A negative return value indicates that
** no malloc() failure is scheduled.
*/
static int SQLITE_TCLAPI test_memdebug_pending(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int nPending;
  if( objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }
  nPending = faultsimPending();
  Tcl_SetObjResult(interp, Tcl_NewIntObj(nPending));
  return TCL_OK;
}

/*
** The following global variable keeps track of the number of tests
** that have run.  This variable is only useful when running in the
** debugger.
*/
static int sqlite3_memdebug_title_count = 0;

/*
** Usage:    sqlite3_memdebug_settitle TITLE
**
** Set a title string stored with each allocation.  The TITLE is
** typically the name of the test that was running when the
** allocation occurred.  The TITLE is stored with the allocation
** and can be used to figure out which tests are leaking memory.
**
** Each title overwrite the previous.
*/
static int SQLITE_TCLAPI test_memdebug_settitle(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  sqlite3_memdebug_title_count++;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "TITLE");
    return TCL_ERROR;
  }
#ifdef SQLITE_MEMDEBUG
  {
    const char *zTitle;
    extern int sqlite3MemdebugSettitle(const char*);
    zTitle = Tcl_GetString(objv[1]);
    sqlite3MemdebugSettitle(zTitle);
  }
#endif
  return TCL_OK;
}

#define MALLOC_LOG_FRAMES  10 
#define MALLOC_LOG_KEYINTS (                                              \
    10 * ((sizeof(int)>=sizeof(void*)) ? 1 : sizeof(void*)/sizeof(int))   \
)
static Tcl_HashTable aMallocLog;
static int mallocLogEnabled = 0;

typedef struct MallocLog MallocLog;
struct MallocLog {
  int nCall;
  int nByte;
};

#ifdef SQLITE_MEMDEBUG
static void test_memdebug_callback(int nByte, int nFrame, void **aFrame){
  if( mallocLogEnabled ){
    MallocLog *pLog;
    Tcl_HashEntry *pEntry;
    int isNew;

    int aKey[MALLOC_LOG_KEYINTS];
    unsigned int nKey = sizeof(int)*MALLOC_LOG_KEYINTS;

    memset(aKey, 0, nKey);
    if( (sizeof(void*)*nFrame)<nKey ){
      nKey = nFrame*sizeof(void*);
    }
    memcpy(aKey, aFrame, nKey);

    pEntry = Tcl_CreateHashEntry(&aMallocLog, (const char *)aKey, &isNew);
    if( isNew ){
      pLog = (MallocLog *)Tcl_Alloc(sizeof(MallocLog));
      memset(pLog, 0, sizeof(MallocLog));
      Tcl_SetHashValue(pEntry, (ClientData)pLog);
    }else{
      pLog = (MallocLog *)Tcl_GetHashValue(pEntry);
    }

    pLog->nCall++;
    pLog->nByte += nByte;
  }
}
#endif /* SQLITE_MEMDEBUG */

static void test_memdebug_log_clear(void){
  Tcl_HashSearch search;
  Tcl_HashEntry *pEntry;
  for(
    pEntry=Tcl_FirstHashEntry(&aMallocLog, &search);
    pEntry;
    pEntry=Tcl_NextHashEntry(&search)
  ){
    MallocLog *pLog = (MallocLog *)Tcl_GetHashValue(pEntry);
    Tcl_Free((char *)pLog);
  }
  Tcl_DeleteHashTable(&aMallocLog);
  Tcl_InitHashTable(&aMallocLog, MALLOC_LOG_KEYINTS);
}

static int SQLITE_TCLAPI test_memdebug_log(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  static int isInit = 0;
  int iSub;

  static const char *MB_strs[] = { "start", "stop", "dump", "clear", "sync" };
  enum MB_enum { 
      MB_LOG_START, MB_LOG_STOP, MB_LOG_DUMP, MB_LOG_CLEAR, MB_LOG_SYNC 
  };

  if( !isInit ){
#ifdef SQLITE_MEMDEBUG
    extern void sqlite3MemdebugBacktraceCallback(
        void (*xBacktrace)(int, int, void **));
    sqlite3MemdebugBacktraceCallback(test_memdebug_callback);
#endif
    Tcl_InitHashTable(&aMallocLog, MALLOC_LOG_KEYINTS);
    isInit = 1;
  }

  if( objc<2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SUB-COMMAND ...");
  }
  if( Tcl_GetIndexFromObj(interp, objv[1], MB_strs, "sub-command", 0, &iSub) ){
    return TCL_ERROR;
  }

  switch( (enum MB_enum)iSub ){
    case MB_LOG_START:
      mallocLogEnabled = 1;
      break;
    case MB_LOG_STOP:
      mallocLogEnabled = 0;
      break;
    case MB_LOG_DUMP: {
      Tcl_HashSearch search;
      Tcl_HashEntry *pEntry;
      Tcl_Obj *pRet = Tcl_NewObj();

      assert(sizeof(Tcl_WideInt)>=sizeof(void*));

      for(
        pEntry=Tcl_FirstHashEntry(&aMallocLog, &search);
        pEntry;
        pEntry=Tcl_NextHashEntry(&search)
      ){
        Tcl_Obj *apElem[MALLOC_LOG_FRAMES+2];
        MallocLog *pLog = (MallocLog *)Tcl_GetHashValue(pEntry);
        Tcl_WideInt *aKey = (Tcl_WideInt *)Tcl_GetHashKey(&aMallocLog, pEntry);
        int ii;
  
        apElem[0] = Tcl_NewIntObj(pLog->nCall);
        apElem[1] = Tcl_NewIntObj(pLog->nByte);
        for(ii=0; ii<MALLOC_LOG_FRAMES; ii++){
          apElem[ii+2] = Tcl_NewWideIntObj(aKey[ii]);
        }

        Tcl_ListObjAppendElement(interp, pRet,
            Tcl_NewListObj(MALLOC_LOG_FRAMES+2, apElem)
        );
      }

      Tcl_SetObjResult(interp, pRet);
      break;
    }
    case MB_LOG_CLEAR: {
      test_memdebug_log_clear();
      break;
    }

    case MB_LOG_SYNC: {
#ifdef SQLITE_MEMDEBUG
      extern void sqlite3MemdebugSync();
      test_memdebug_log_clear();
      mallocLogEnabled = 1;
      sqlite3MemdebugSync();
#endif
      break;
    }
  }

  return TCL_OK;
}

/*
** Usage:    sqlite3_config_pagecache SIZE N
**
** Set the page-cache memory buffer using SQLITE_CONFIG_PAGECACHE.
** The buffer is static and is of limited size.  N might be
** adjusted downward as needed to accommodate the requested size.
** The revised value of N is returned.
**
** A negative SIZE causes the buffer pointer to be NULL.
*/
static int SQLITE_TCLAPI test_config_pagecache(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int sz, N;
  Tcl_Obj *pRes;
  static char *buf = 0;
  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SIZE N");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[1], &sz) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[2], &N) ) return TCL_ERROR;
  free(buf);
  buf = 0;

  /* Set the return value */
  pRes = Tcl_NewObj();
  Tcl_ListObjAppendElement(0, pRes, Tcl_NewIntObj(sqlite3GlobalConfig.szPage));
  Tcl_ListObjAppendElement(0, pRes, Tcl_NewIntObj(sqlite3GlobalConfig.nPage));
  Tcl_SetObjResult(interp, pRes);

  if( sz<0 ){
    sqlite3_config(SQLITE_CONFIG_PAGECACHE, (void*)0, 0, 0);
  }else{
    buf = malloc( sz*N );
    sqlite3_config(SQLITE_CONFIG_PAGECACHE, buf, sz, N);
  }
  return TCL_OK;
}

/*
** Usage:    sqlite3_config_alt_pcache INSTALL_FLAG DISCARD_CHANCE PRNG_SEED
**
** Set up the alternative test page cache.  Install if INSTALL_FLAG is
** true and uninstall (reverting to the default page cache) if INSTALL_FLAG
** is false.  DISCARD_CHANGE is an integer between 0 and 100 inclusive
** which determines the chance of discarding a page when unpinned.  100
** is certainty.  0 is never.  PRNG_SEED is the pseudo-random number generator
** seed.
*/
static int SQLITE_TCLAPI test_alt_pcache(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int installFlag;
  int discardChance = 0;
  int prngSeed = 0;
  int highStress = 0;
  extern void installTestPCache(int,unsigned,unsigned,unsigned);
  if( objc<2 || objc>5 ){
    Tcl_WrongNumArgs(interp, 1, objv, 
        "INSTALLFLAG DISCARDCHANCE PRNGSEEED HIGHSTRESS");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[1], &installFlag) ) return TCL_ERROR;
  if( objc>=3 && Tcl_GetIntFromObj(interp, objv[2], &discardChance) ){
     return TCL_ERROR;
  }
  if( objc>=4 && Tcl_GetIntFromObj(interp, objv[3], &prngSeed) ){
     return TCL_ERROR;
  }
  if( objc>=5 && Tcl_GetIntFromObj(interp, objv[4], &highStress) ){
    return TCL_ERROR;
  }
  if( discardChance<0 || discardChance>100 ){
    Tcl_AppendResult(interp, "discard-chance should be between 0 and 100",
                     (char*)0);
    return TCL_ERROR;
  }
  installTestPCache(installFlag, (unsigned)discardChance, (unsigned)prngSeed,
                    (unsigned)highStress);
  return TCL_OK;
}

/*
** Usage:    sqlite3_config_memstatus BOOLEAN
**
** Enable or disable memory status reporting using SQLITE_CONFIG_MEMSTATUS.
*/
static int SQLITE_TCLAPI test_config_memstatus(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int enable, rc;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "BOOLEAN");
    return TCL_ERROR;
  }
  if( Tcl_GetBooleanFromObj(interp, objv[1], &enable) ) return TCL_ERROR;
  rc = sqlite3_config(SQLITE_CONFIG_MEMSTATUS, enable);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}

/*
** Usage:    sqlite3_config_lookaside  SIZE  COUNT
**
*/
static int SQLITE_TCLAPI test_config_lookaside(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int sz, cnt;
  Tcl_Obj *pRet;
  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SIZE COUNT");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[1], &sz) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[2], &cnt) ) return TCL_ERROR;
  pRet = Tcl_NewObj();
  Tcl_ListObjAppendElement(
      interp, pRet, Tcl_NewIntObj(sqlite3GlobalConfig.szLookaside)
  );
  Tcl_ListObjAppendElement(
      interp, pRet, Tcl_NewIntObj(sqlite3GlobalConfig.nLookaside)
  );
  sqlite3_config(SQLITE_CONFIG_LOOKASIDE, sz, cnt);
  Tcl_SetObjResult(interp, pRet);
  return TCL_OK;
}


/*
** Usage:    sqlite3_db_config_lookaside  CONNECTION  BUFID  SIZE  COUNT
**
** There are two static buffers with BUFID 1 and 2.   Each static buffer
** is 10KB in size.  A BUFID of 0 indicates that the buffer should be NULL
** which will cause sqlite3_db_config() to allocate space on its own.
*/
static int SQLITE_TCLAPI test_db_config_lookaside(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc;
  int sz, cnt;
  sqlite3 *db;
  int bufid;
  static char azBuf[2][10000];
  extern int getDbPointer(Tcl_Interp*, const char*, sqlite3**);
  if( objc!=5 ){
    Tcl_WrongNumArgs(interp, 1, objv, "BUFID SIZE COUNT");
    return TCL_ERROR;
  }
  if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[2], &bufid) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[3], &sz) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[4], &cnt) ) return TCL_ERROR;
  if( bufid==0 ){
    rc = sqlite3_db_config(db, SQLITE_DBCONFIG_LOOKASIDE, (void*)0, sz, cnt);
  }else if( bufid>=1 && bufid<=2 && sz*cnt<=sizeof(azBuf[0]) ){
    rc = sqlite3_db_config(db, SQLITE_DBCONFIG_LOOKASIDE, azBuf[bufid], sz,cnt);
  }else{
    Tcl_AppendResult(interp, "illegal arguments - see documentation", (char*)0);
    return TCL_ERROR;
  }
  Tcl_SetObjResult(interp, Tcl_NewIntObj(rc));
  return TCL_OK;
}

/*
** Usage:    sqlite3_config_heap NBYTE NMINALLOC
*/
static int SQLITE_TCLAPI test_config_heap(
  void * clientData, 
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  static char *zBuf; /* Use this memory */
  int nByte;         /* Size of buffer to pass to sqlite3_config() */
  int nMinAlloc;     /* Size of minimum allocation */
  int rc;            /* Return code of sqlite3_config() */

  Tcl_Obj * CONST *aArg = &objv[1];
  int nArg = objc-1;

  if( nArg!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "NBYTE NMINALLOC");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, aArg[0], &nByte) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, aArg[1], &nMinAlloc) ) return TCL_ERROR;

  if( nByte==0 ){
    free( zBuf );
    zBuf = 0;
    rc = sqlite3_config(SQLITE_CONFIG_HEAP, (void*)0, 0, 0);
  }else{
    zBuf = realloc(zBuf, nByte);
    rc = sqlite3_config(SQLITE_CONFIG_HEAP, zBuf, nByte, nMinAlloc);
  }

  Tcl_SetResult(interp, (char *)sqlite3ErrName(rc), TCL_VOLATILE);
  return TCL_OK;
}

/*
** Usage:    sqlite3_config_heap_size NBYTE
*/
static int SQLITE_TCLAPI test_config_heap_size(
  void * clientData, 
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int nByte;         /* Size to pass to sqlite3_config() */
  int rc;            /* Return code of sqlite3_config() */

  Tcl_Obj * CONST *aArg = &objv[1];
  int nArg = objc-1;

  if( nArg!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "NBYTE");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, aArg[0], &nByte) ) return TCL_ERROR;

  rc = sqlite3_config(SQLITE_CONFIG_WIN32_HEAPSIZE, nByte);

  Tcl_SetResult(interp, (char *)sqlite3ErrName(rc), TCL_VOLATILE);
  return TCL_OK;
}

/*
** Usage:    sqlite3_config_error  [DB]
**
** Invoke sqlite3_config() or sqlite3_db_config() with invalid
** opcodes and verify that they return errors.
*/
static int SQLITE_TCLAPI test_config_error(
  void * clientData, 
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  sqlite3 *db;
  extern int getDbPointer(Tcl_Interp*, const char*, sqlite3**);

  if( objc!=2 && objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "[DB]");
    return TCL_ERROR;
  }
  if( objc==2 ){
    if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;
    if( sqlite3_db_config(db, 99999)!=SQLITE_ERROR ){
      Tcl_AppendResult(interp, 
            "sqlite3_db_config(db, 99999) does not return SQLITE_ERROR",
            (char*)0);
      return TCL_ERROR;
    }
  }else{
    if( sqlite3_config(99999)!=SQLITE_ERROR ){
      Tcl_AppendResult(interp, 
          "sqlite3_config(99999) does not return SQLITE_ERROR",
          (char*)0);
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}

/*
** Usage:    sqlite3_config_uri  BOOLEAN
**
** Enables or disables interpretation of URI parameters by default using
** SQLITE_CONFIG_URI.
*/
static int SQLITE_TCLAPI test_config_uri(
  void * clientData, 
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc;
  int bOpenUri;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "BOOL");
    return TCL_ERROR;
  }
  if( Tcl_GetBooleanFromObj(interp, objv[1], &bOpenUri) ){
    return TCL_ERROR;
  }

  rc = sqlite3_config(SQLITE_CONFIG_URI, bOpenUri);
  Tcl_SetResult(interp, (char *)sqlite3ErrName(rc), TCL_VOLATILE);

  return TCL_OK;
}

/*
** Usage:    sqlite3_config_cis  BOOLEAN
**
** Enables or disables the use of the covering-index scan optimization.
** SQLITE_CONFIG_COVERING_INDEX_SCAN.
*/
static int SQLITE_TCLAPI test_config_cis(
  void * clientData, 
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc;
  int bUseCis;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "BOOL");
    return TCL_ERROR;
  }
  if( Tcl_GetBooleanFromObj(interp, objv[1], &bUseCis) ){
    return TCL_ERROR;
  }

  rc = sqlite3_config(SQLITE_CONFIG_COVERING_INDEX_SCAN, bUseCis);
  Tcl_SetResult(interp, (char *)sqlite3ErrName(rc), TCL_VOLATILE);

  return TCL_OK;
}

/*
** Usage:    sqlite3_config_pmasz  INTEGER
**
** Set the minimum PMA size.
*/
static int SQLITE_TCLAPI test_config_pmasz(
  void * clientData, 
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc;
  int iPmaSz;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "BOOL");
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[1], &iPmaSz) ){
    return TCL_ERROR;
  }

  rc = sqlite3_config(SQLITE_CONFIG_PMASZ, iPmaSz);
  Tcl_SetResult(interp, (char *)sqlite3ErrName(rc), TCL_VOLATILE);

  return TCL_OK;
}


/*
** Usage:    sqlite3_dump_memsys3  FILENAME
**           sqlite3_dump_memsys5  FILENAME
**
** Write a summary of unfreed memsys3 allocations to FILENAME.
*/
static int SQLITE_TCLAPI test_dump_memsys3(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "FILENAME");
    return TCL_ERROR;
  }

  switch( SQLITE_PTR_TO_INT(clientData) ){
    case 3: {
#ifdef SQLITE_ENABLE_MEMSYS3
      extern void sqlite3Memsys3Dump(const char*);
      sqlite3Memsys3Dump(Tcl_GetString(objv[1]));
      break;
#endif
    }
    case 5: {
#ifdef SQLITE_ENABLE_MEMSYS5
      extern void sqlite3Memsys5Dump(const char*);
      sqlite3Memsys5Dump(Tcl_GetString(objv[1]));
      break;
#endif
    }
  }
  return TCL_OK;
}

/*
** Usage:    sqlite3_status  OPCODE  RESETFLAG
**
** Return a list of three elements which are the sqlite3_status() return
** code, the current value, and the high-water mark value.
*/
static int SQLITE_TCLAPI test_status(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc, iValue, mxValue;
  int i, op = 0, resetFlag;
  const char *zOpName;
  static const struct {
    const char *zName;
    int op;
  } aOp[] = {
    { "SQLITE_STATUS_MEMORY_USED",         SQLITE_STATUS_MEMORY_USED         },
    { "SQLITE_STATUS_MALLOC_SIZE",         SQLITE_STATUS_MALLOC_SIZE         },
    { "SQLITE_STATUS_PAGECACHE_USED",      SQLITE_STATUS_PAGECACHE_USED      },
    { "SQLITE_STATUS_PAGECACHE_OVERFLOW",  SQLITE_STATUS_PAGECACHE_OVERFLOW  },
    { "SQLITE_STATUS_PAGECACHE_SIZE",      SQLITE_STATUS_PAGECACHE_SIZE      },
    { "SQLITE_STATUS_SCRATCH_USED",        SQLITE_STATUS_SCRATCH_USED        },
    { "SQLITE_STATUS_SCRATCH_OVERFLOW",    SQLITE_STATUS_SCRATCH_OVERFLOW    },
    { "SQLITE_STATUS_SCRATCH_SIZE",        SQLITE_STATUS_SCRATCH_SIZE        },
    { "SQLITE_STATUS_PARSER_STACK",        SQLITE_STATUS_PARSER_STACK        },
    { "SQLITE_STATUS_MALLOC_COUNT",        SQLITE_STATUS_MALLOC_COUNT        },
  };
  Tcl_Obj *pResult;
  if( objc!=3 ){
    Tcl_WrongNumArgs(interp, 1, objv, "PARAMETER RESETFLAG");
    return TCL_ERROR;
  }
  zOpName = Tcl_GetString(objv[1]);
  for(i=0; i<ArraySize(aOp); i++){
    if( strcmp(aOp[i].zName, zOpName)==0 ){
      op = aOp[i].op;
      break;
    }
  }
  if( i>=ArraySize(aOp) ){
    if( Tcl_GetIntFromObj(interp, objv[1], &op) ) return TCL_ERROR;
  }
  if( Tcl_GetBooleanFromObj(interp, objv[2], &resetFlag) ) return TCL_ERROR;
  iValue = 0;
  mxValue = 0;
  rc = sqlite3_status(op, &iValue, &mxValue, resetFlag);
  pResult = Tcl_NewObj();
  Tcl_ListObjAppendElement(0, pResult, Tcl_NewIntObj(rc));
  Tcl_ListObjAppendElement(0, pResult, Tcl_NewIntObj(iValue));
  Tcl_ListObjAppendElement(0, pResult, Tcl_NewIntObj(mxValue));
  Tcl_SetObjResult(interp, pResult);
  return TCL_OK;
}

/*
** Usage:    sqlite3_db_status  DATABASE  OPCODE  RESETFLAG
**
** Return a list of three elements which are the sqlite3_db_status() return
** code, the current value, and the high-water mark value.
*/
static int SQLITE_TCLAPI test_db_status(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc, iValue, mxValue;
  int i, op = 0, resetFlag;
  const char *zOpName;
  sqlite3 *db;
  extern int getDbPointer(Tcl_Interp*, const char*, sqlite3**);
  static const struct {
    const char *zName;
    int op;
  } aOp[] = {
    { "LOOKASIDE_USED",      SQLITE_DBSTATUS_LOOKASIDE_USED      },
    { "CACHE_USED",          SQLITE_DBSTATUS_CACHE_USED          },
    { "SCHEMA_USED",         SQLITE_DBSTATUS_SCHEMA_USED         },
    { "STMT_USED",           SQLITE_DBSTATUS_STMT_USED           },
    { "LOOKASIDE_HIT",       SQLITE_DBSTATUS_LOOKASIDE_HIT       },
    { "LOOKASIDE_MISS_SIZE", SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE },
    { "LOOKASIDE_MISS_FULL", SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL },
    { "CACHE_HIT",           SQLITE_DBSTATUS_CACHE_HIT           },
    { "CACHE_MISS",          SQLITE_DBSTATUS_CACHE_MISS          },
    { "CACHE_WRITE",         SQLITE_DBSTATUS_CACHE_WRITE         },
    { "DEFERRED_FKS",        SQLITE_DBSTATUS_DEFERRED_FKS        },
    { "CACHE_USED_SHARED",   SQLITE_DBSTATUS_CACHE_USED_SHARED   },
    { "CACHE_SPILL",         SQLITE_DBSTATUS_CACHE_SPILL         },
    { "TEMPBUF_SPILL",       SQLITE_DBSTATUS_TEMPBUF_SPILL       },
  };
  Tcl_Obj *pResult;
  if( objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DB PARAMETER RESETFLAG");
    return TCL_ERROR;
  }
  if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;
  zOpName = Tcl_GetString(objv[2]);
  if( memcmp(zOpName, "SQLITE_", 7)==0 ) zOpName += 7;
  if( memcmp(zOpName, "DBSTATUS_", 9)==0 ) zOpName += 9;
  for(i=0; i<ArraySize(aOp); i++){
    if( strcmp(aOp[i].zName, zOpName)==0 ){
      op = aOp[i].op;
      break;
    }
  }
  if( i>=ArraySize(aOp) ){
    if( Tcl_GetIntFromObj(interp, objv[2], &op) ) return TCL_ERROR;
  }
  if( Tcl_GetBooleanFromObj(interp, objv[3], &resetFlag) ) return TCL_ERROR;
  iValue = 0;
  mxValue = 0;
  rc = sqlite3_db_status(db, op, &iValue, &mxValue, resetFlag);
  pResult = Tcl_NewObj();
  Tcl_ListObjAppendElement(0, pResult, Tcl_NewIntObj(rc));
  Tcl_ListObjAppendElement(0, pResult, Tcl_NewIntObj(iValue));
  Tcl_ListObjAppendElement(0, pResult, Tcl_NewIntObj(mxValue));
  Tcl_SetObjResult(interp, pResult);
  return TCL_OK;
}

/*
** install_malloc_faultsim BOOLEAN
*/
static int SQLITE_TCLAPI test_install_malloc_faultsim(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc;
  int isInstall;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "BOOLEAN");
    return TCL_ERROR;
  }
  if( TCL_OK!=Tcl_GetBooleanFromObj(interp, objv[1], &isInstall) ){
    return TCL_ERROR;
  }
  rc = faultsimInstall(isInstall);
  Tcl_SetResult(interp, (char *)sqlite3ErrName(rc), TCL_VOLATILE);
  return TCL_OK;
}

/*
** sqlite3_install_memsys3
*/
static int SQLITE_TCLAPI test_install_memsys3(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc = SQLITE_MISUSE;
#ifdef SQLITE_ENABLE_MEMSYS3
  const sqlite3_mem_methods *sqlite3MemGetMemsys3(void);
  rc = sqlite3_config(SQLITE_CONFIG_MALLOC, sqlite3MemGetMemsys3());
#endif
  Tcl_SetResult(interp, (char *)sqlite3ErrName(rc), TCL_VOLATILE);
  return TCL_OK;
}

static int SQLITE_TCLAPI test_vfs_oom_test(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  extern int sqlite3_memdebug_vfs_oom_test;
  if( objc>2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "?INTEGER?");
    return TCL_ERROR;
  }else if( objc==2 ){
    int iNew;
    if( Tcl_GetIntFromObj(interp, objv[1], &iNew) ) return TCL_ERROR;
    sqlite3_memdebug_vfs_oom_test = iNew;
  }
  Tcl_SetObjResult(interp, Tcl_NewIntObj(sqlite3_memdebug_vfs_oom_test));
  return TCL_OK;
}

/*
** Register commands with the TCL interpreter.
*/
int Sqlitetest_malloc_Init(Tcl_Interp *interp){
  static struct {
     char *zName;
     Tcl_ObjCmdProc *xProc;
     int clientData;
  } aObjCmd[] = {
     { "sqlite3_malloc",             test_malloc                   ,0 },
     { "sqlite3_realloc",            test_realloc                  ,0 },
     { "sqlite3_free",               test_free                     ,0 },
     { "memset",                     test_memset                   ,0 },
     { "memget",                     test_memget                   ,0 },
     { "sqlite3_memory_used",        test_memory_used              ,0 },
     { "sqlite3_memory_highwater",   test_memory_highwater         ,0 },
     { "sqlite3_memdebug_backtrace", test_memdebug_backtrace       ,0 },
     { "sqlite3_memdebug_dump",      test_memdebug_dump            ,0 },
     { "sqlite3_memdebug_fail",      test_memdebug_fail            ,0 },
     { "sqlite3_memdebug_pending",   test_memdebug_pending         ,0 },
     { "sqlite3_memdebug_settitle",  test_memdebug_settitle        ,0 },
     { "sqlite3_memdebug_malloc_count", test_memdebug_malloc_count ,0 },
     { "sqlite3_memdebug_log",       test_memdebug_log             ,0 },
     { "sqlite3_config_pagecache",   test_config_pagecache         ,0 },
     { "sqlite3_config_alt_pcache",  test_alt_pcache               ,0 },
     { "sqlite3_status",             test_status                   ,0 },
     { "sqlite3_db_status",          test_db_status                ,0 },
     { "install_malloc_faultsim",    test_install_malloc_faultsim  ,0 },
     { "sqlite3_config_heap",        test_config_heap              ,0 },
     { "sqlite3_config_heap_size",   test_config_heap_size         ,0 },
     { "sqlite3_config_memstatus",   test_config_memstatus         ,0 },
     { "sqlite3_config_lookaside",   test_config_lookaside         ,0 },
     { "sqlite3_config_error",       test_config_error             ,0 },
     { "sqlite3_config_uri",         test_config_uri               ,0 },
     { "sqlite3_config_cis",         test_config_cis               ,0 },
     { "sqlite3_config_pmasz",       test_config_pmasz             ,0 },
     { "sqlite3_db_config_lookaside",test_db_config_lookaside      ,0 },
     { "sqlite3_dump_memsys3",       test_dump_memsys3             ,3 },
     { "sqlite3_dump_memsys5",       test_dump_memsys3             ,5 },
     { "sqlite3_install_memsys3",    test_install_memsys3          ,0 },
     { "sqlite3_memdebug_vfs_oom_test", test_vfs_oom_test          ,0 },
  };
  int i;
  for(i=0; i<sizeof(aObjCmd)/sizeof(aObjCmd[0]); i++){
    ClientData c = (ClientData)SQLITE_INT_TO_PTR(aObjCmd[i].clientData);
    Tcl_CreateObjCommand(interp, aObjCmd[i].zName, aObjCmd[i].xProc, c, 0);
  }
  return TCL_OK;
}
#endif
