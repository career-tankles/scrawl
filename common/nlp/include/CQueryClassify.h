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

//queryʱ�����ж����id����
#define QUERY_TIME_TYPE_NO		8000	//��ʱЧ��
#define QUERY_TIME_TYPE_HOT		8001	//�ȵ�
#define QUERY_TIME_TYPE_PERIOD	8002	//������
#define QUERY_TIME_TYPE_CLOSER	8003	//Խ��Խ��
#define QUERY_TIME_TYPE_EXPIRED	8004	//���ڲ���
namespace query_classify_rule {
//���ŵȼ�
enum GradeType {
  GRADE_TYPE_NONE = 0,
  GRADE_TYPE_RARE = 1,
  GRADE_TYPE_NOMAL = 2,
  GRADE_TYPE_HOT = 3,
};

//�ȶ�
enum HotType {
  HOT_TYPE_NONE = 0,
  HOT_TYPE_LOW = 1,
  HOT_TYPE_MID = 2,
  HOT_TYPE_HIGH = 3,
  HOT_TYPE_HIGHER = 4,
};

//ʵ��
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
 *�������Ľṹ��
 */
struct TypeInfo {
  int typeId;             //���ID
  char* rule;         	  //��������������
  GradeType conf;				  //���ŵȼ�
  HotType hot;				  //�ȶ�
  EntityInfo *entityArray;  //ʵ����Ϣ
  unsigned int entityCount;  //ʵ��ĸ���
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
 * 		     ����ĳ�ʼ��
 * Parameter:
 * 		configPath [I] �����ļ���·��
 * 		is_binary  [I] 
 * return: ����ִ�з��������� Ϊ�ճ�ʼ��ʧ��
 */
void* InitQueryClassify(const char *configPath, bool is_binary = true);

/**
 * Founction:ReLoad
 * 			���³�ʼ��
 * Parameter:
 * 		configPath [I] �����ļ���·��
 * 		is_binary  [I]
 * 		qcdic		   ��Ҫ���¼��صķ��������
 * 	return: �Ƿ�ɹ����³�ʼ��
 */
//bool ReLoad(const char *configPath, bool is_binary, void* qcdic);
/**
 * Founction:CreateClassifyQueryResult
 * 			Ϊ�����������ڴ�
 * Parameter:
 * 		maxClassSize [I] ������ֳ��������Ŀ
 * 	return: ����ṹ���
 */
TypeInfoVec* CreateClassifyQueryResult(int maxClassSize = 24);

/**
 * Founction:DestroyClassifyQueryResult
 * 			������ʹ����Ϻ���Ҫ����
 * Parameter:
 * 		typeInfoRes [I]Ҫ�����ٵķ������
 * 	return: NULL
 */
void DestroyClassifyQueryResult(TypeInfoVec* typeInfoRes,
                                int maxClassSize = 24);
/**
 * Founction:ClassifyQuery
 *           ����ĺ���
 * Parameter:
 * 		globleHandle [I]  �ɳ�ʼ�����ຯ��InitQueryClassify ���ض���
 * 		query        [I]  �����query
 * 		psr          [I]  �ִʽ����ǰΪ������ȵķִʽ��
 * 		entityInfo   [I]  �ִʵ�ʵ����
 * 		TypeInfoVec  [O]  �������Ľṹ������
 * 		nentity      [I]  ʵ��ĸ���
 * return: ���������������-1��ʾ����ʧ��
 */
int ClassifyQuery(void *globleHandle, const char *query, SegmentResult *psr,
                  EntityResult* entityInfo, TypeInfoVec *typeInfoRes,
                  int nentity);

/**
 * Founction:FreeQueryClassify
 *           �ͷŷ���������
 * Parameter:
 *      globleHandle [I] ָ�����������
 * return: 
 */
int FreeQueryClassify(void* globleHandle);

} /* namespace query_classify_rule */

#endif /* CQUERYCLASSIFY_H_ */
