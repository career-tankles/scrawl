/*
 * CQueryClassify.h
 *
 *  Created on: 2012-12-13
 *      Author: root
 */

#ifndef CQUERYCLASSIFY_H_
#define CQUERYCLASSIFY_H_
#include <string.h>
#include "../include/EasouSeg.h"
#define QUERY_ENTITY_MAX_COUNT  15
#define QUERY_DEMAND_MAX_COUNT  5
#define QUERY_SUPERID_MAX_COUNT 15

//query时间敏感度类别id定义
#define QUERY_TIME_TYPE_NO		8000	//无时效性
#define QUERY_TIME_TYPE_HOT		8001	//热点
#define QUERY_TIME_TYPE_PERIOD	8002	//周期性
#define QUERY_TIME_TYPE_CLOSER	8003	//越近越好
#define QUERY_TIME_TYPE_EXPIRED	8004	//过期不行
namespace query_classify_rule {
//置信等级
enum GradeType {
  GRADE_TYPE_NONE = 0,
  GRADE_TYPE_RARE = 1,
  GRADE_TYPE_NOMAL = 2,
  GRADE_TYPE_HOT = 3,
};

//热度
enum HotType {
  HOT_TYPE_NONE = 0,
  HOT_TYPE_LOW = 1,
  HOT_TYPE_MID = 2,
  HOT_TYPE_HIGH = 3,
  HOT_TYPE_HIGHER = 4,
};

//实体
struct EntityInfo {
  char* entity;
  int offset :24;
  int length :8;
  unsigned int feature;

  EntityInfo() {
    entity = NULL;
    offset = 0;
    length = 0;
    feature = 0;
  }
  ~EntityInfo() {
    if (NULL != entity) {
      delete entity;
      entity = NULL;
    }
  }
};

/*
 *输出结果的结构体
 */
struct TypeInfo {
  int typeId;             //类别ID
  char* rule;         	  //被哪条规则命中
  GradeType conf;				  //置信等级
  HotType hot;				  //热度
  EntityInfo *entityArray;  //实体信息
  unsigned int entityCount;  //实体的个数
  int superId[QUERY_SUPERID_MAX_COUNT];
  char* demandWords[QUERY_DEMAND_MAX_COUNT];
  TypeInfo() {
    typeId = 0;
    rule = NULL;
    conf = GRADE_TYPE_HOT;
    hot = HOT_TYPE_LOW;
    entityArray = NULL;
    entityCount = 0;
    for (int i = 0; i < QUERY_SUPERID_MAX_COUNT; i++) {
      superId[i] = 0;
    }
    for (int i = 0; i < QUERY_DEMAND_MAX_COUNT; i++) {
      demandWords[i] = NULL;
    }
  }
  ~TypeInfo() {
    if (NULL != rule) {
      delete rule;
      rule = NULL;
    }
    if (NULL != entityArray) {
      delete[] entityArray;
      entityArray = NULL;
    }
    entityCount = 0;
  }
};

struct TypeInfoVec {
  TypeInfo * type_info;
  int super_type[QUERY_SUPERID_MAX_COUNT];
  int type_count;
  int max_count;

  TypeInfoVec() {
    type_info = NULL;
    type_count = 0;
    max_count = 0;
    for (int i = 0; i < QUERY_SUPERID_MAX_COUNT; i++) {
      super_type[i] = 0;
    }
  }

  TypeInfoVec(TypeInfo * t, int c) {
    type_info = t;
    type_count = c;
    max_count = 0;
  }
};
/**
 * Founction:InitQueryClassify
 * 		     分类的初始化
 * Parameter:
 * 		configPath [I] 配置文件的路径
 * 		is_binary  [I] 
 * return: 返回执行分类的类对象 为空初始化失败
 */
void* InitQueryClassify(const char *configPath, bool is_binary = true);

/**
 * Founction:ReLoad
 * 			重新初始化
 * Parameter:
 * 		configPath [I] 配置文件的路径
 * 		is_binary  [I]
 * 		qcdic		   需要重新加载的分类类对象
 * 	return: 是否成功重新初始化
 */
//bool ReLoad(const char *configPath, bool is_binary, void* qcdic);
/**
 * Founction:CreateClassifyQueryResult
 * 			为分类结果分配内存
 * Parameter:
 * 		maxClassSize [I] 最多分类分出的类别数目
 * 	return: 分类结构句柄
 */
TypeInfoVec* CreateClassifyQueryResult(int maxClassSize = 24);

/**
 * Founction:DestroyClassifyQueryResult
 * 			分类结果使用完毕后需要销毁
 * Parameter:
 * 		typeInfoRes [I]要被销毁的分类对象
 * 	return: NULL
 */
void DestroyClassifyQueryResult(TypeInfoVec* typeInfoRes,
                                int maxClassSize = 24);
/**
 * Founction:ClassifyQuery
 *           分类的函数
 * Parameter:
 * 		globleHandle [I]  由初始化分类函数InitQueryClassify 返回对象
 * 		query        [I]  分类的query
 * 		psr          [I]  分词结果当前为混合粒度的分词结果
 * 		entityInfo   [I]  分词的实体结果
 * 		TypeInfoVec  [O]  分类结果的结构体数组
 * 		nentity      [I]  实体的个数
 * return: 分类结果的类别数，-1表示分类失败
 */
int ClassifyQuery(void *globleHandle, const char *query, SegmentResult *psr,
                  EntityResult* entityInfo, TypeInfoVec *typeInfoRes,
                  int nentity);

/**
 * Founction:FreeQueryClassify
 *           释放分类的类对象
 * Parameter:
 *      globleHandle [I] 指向分类的类对象
 * return: 
 */
int FreeQueryClassify(void* globleHandle);

} /* namespace query_classify_rule */

#endif /* CQUERYCLASSIFY_H_ */
