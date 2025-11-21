/*
** 2010 August 30
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
*/

#ifndef _SQLITE3RTREE_H_
#define _SQLITE3RTREE_H_

/*
** SQLite R-Tree 扩展公共接口头文件
**
** 本头文件定义了 SQLite R-Tree 空间索引扩展的公共 API 接口。
** R-Tree 是一种基于 R-Tree 算法的空间数据结构，专门用于高效地
** 存储和查询多维空间数据。
**
** R-Tree 扩展核心特性：
**
** 1. 空间索引结构：
**    - 基于 R-Tree 算法的层次化空间索引
**    - 支持 1-5 维空间数据存储
**    - 最小边界矩形 (MBR) 索引技术
**    - 动态插入、删除和更新操作
**
** 2. 查询功能：
**    - 包含查询 (Contains)：完全包含在指定区域内的对象
**    - 相交查询 (Intersects)：与指定区域相交的对象
**    - 邻近查询 (Within)：在指定距离内的对象
**    - 自定义几何查询：用户定义的空间查询函数
**
** 3. 性能优化：
**    - 空间局部性优化
**    - 批量操作支持
**    - 查询计划优化
**    - 内存和磁盘混合存储
**
** 4. 应用场景：
**    - 地理信息系统 (GIS)
**    - 位置服务和导航
**    - 空间数据分析
**    - 计算机图形学
**    - 科学可视化
**
** 使用示例：
** ```sql
** -- 创建 2D 空间索引表
** CREATE VIRTUAL TABLE boundaries USING rtree(
**   id,              -- 整数主键
**   minX, maxX,      -- X 坐标范围
**   minY, maxY       -- Y 坐标范围
** );
**
** -- 插入空间数据
** INSERT INTO boundaries VALUES(1, -100, 100, -50, 50);
**
** -- 空间查询
** SELECT * FROM boundaries
** WHERE minX>=-80 AND maxX<=80 AND minY>=-30 AND maxY<=30;
** ```
*/

#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* R-Tree 几何回调结构体 - 前向声明 */
typedef struct sqlite3_rtree_geometry sqlite3_rtree_geometry;
/* R-Tree 查询信息结构体 - 前向声明 */
typedef struct sqlite3_rtree_query_info sqlite3_rtree_query_info;

/* The double-precision datatype used by RTree depends on the
** SQLITE_RTREE_INT_ONLY compile-time option.
*/
#ifdef SQLITE_RTREE_INT_ONLY
  typedef sqlite3_int64 sqlite3_rtree_dbl;
#else
  typedef double sqlite3_rtree_dbl;
#endif

/*
** Register a geometry callback named zGeom that can be used as part of an
** R-Tree geometry query as follows:
**
**   SELECT ... FROM <rtree> WHERE <rtree col> MATCH $zGeom(... params ...)
*/
int sqlite3_rtree_geometry_callback(
  sqlite3 *db,
  const char *zGeom,
  int (*xGeom)(sqlite3_rtree_geometry*, int, sqlite3_rtree_dbl*,int*),
  void *pContext
);


/*
** A pointer to a structure of the following type is passed as the first
** argument to callbacks registered using rtree_geometry_callback().
*/
struct sqlite3_rtree_geometry {
  void *pContext;                 /* Copy of pContext passed to s_r_g_c() */
  int nParam;                     /* Size of array aParam[] */
  sqlite3_rtree_dbl *aParam;      /* Parameters passed to SQL geom function */
  void *pUser;                    /* Callback implementation user data */
  void (*xDelUser)(void *);       /* Called by SQLite to clean up pUser */
};

/*
** Register a 2nd-generation geometry callback named zScore that can be 
** used as part of an R-Tree geometry query as follows:
**
**   SELECT ... FROM <rtree> WHERE <rtree col> MATCH $zQueryFunc(... params ...)
*/
int sqlite3_rtree_query_callback(
  sqlite3 *db,
  const char *zQueryFunc,
  int (*xQueryFunc)(sqlite3_rtree_query_info*),
  void *pContext,
  void (*xDestructor)(void*)
);


/*
** A pointer to a structure of the following type is passed as the 
** argument to scored geometry callback registered using
** sqlite3_rtree_query_callback().
**
** Note that the first 5 fields of this structure are identical to
** sqlite3_rtree_geometry.  This structure is a subclass of
** sqlite3_rtree_geometry.
*/
struct sqlite3_rtree_query_info {
  void *pContext;                   /* pContext from when function registered */
  int nParam;                       /* Number of function parameters */
  sqlite3_rtree_dbl *aParam;        /* value of function parameters */
  void *pUser;                      /* callback can use this, if desired */
  void (*xDelUser)(void*);          /* function to free pUser */
  sqlite3_rtree_dbl *aCoord;        /* Coordinates of node or entry to check */
  unsigned int *anQueue;            /* Number of pending entries in the queue */
  int nCoord;                       /* Number of coordinates */
  int iLevel;                       /* Level of current node or entry */
  int mxLevel;                      /* The largest iLevel value in the tree */
  sqlite3_int64 iRowid;             /* Rowid for current entry */
  sqlite3_rtree_dbl rParentScore;   /* Score of parent node */
  int eParentWithin;                /* Visibility of parent node */
  int eWithin;                      /* OUT: Visibility */
  sqlite3_rtree_dbl rScore;         /* OUT: Write the score here */
  /* The following fields are only available in 3.8.11 and later */
  sqlite3_value **apSqlParam;       /* Original SQL values of parameters */
};

/*
** Allowed values for sqlite3_rtree_query.eWithin and .eParentWithin.
*/
#define NOT_WITHIN       0   /* Object completely outside of query region */
#define PARTLY_WITHIN    1   /* Object partially overlaps query region */
#define FULLY_WITHIN     2   /* Object fully contained within query region */


#ifdef __cplusplus
}  /* end of the 'extern "C"' block */
#endif

#endif  /* ifndef _SQLITE3RTREE_H_ */
