/*
ver 0.7.0
*/

#ifndef _NLP_ANALYSIS_H
#define _NLP_ANALYSIS_H

#include "../include/EasouSeg.h"
#include "../include/CQueryClassify.h"
#include "../include/SenType.h"
#include "../include/find_syn.h"

using namespace query_classify_rule;
namespace analysis{

//��ʼ���ʵ���ڴ�ռ�ı�ǣ�һ������£����������Ϊһ��
#define SEGMENT_FLAG		1	//segword and entity
#define CLASSIFY_FLAG		2	//class
#define ONLY_SEGMENT_FLAG	4	//only segword
#define SYNONYM_FLAG		8	//synonym
#define QUERY_ENTITY_FLAG 16 //query entity recognition
#define NATURE_ENTITY_FLAG 32 //nature text entity recognition
//nlp��������ṹ��
struct NLPAnalysisInfo{
	SegmentResult*	sr;		//segment result
	EntityResult*	enr;	//entity result
	TypeInfoVec*	cr;		//classify result, sentence type inside
	syn_result*		syr;	//synonym result
	unsigned int 	func_result_flag;	//������Щ���ܿռ䱻��ʼ���ı��
	NLPAnalysisInfo() {
		sr = NULL;
		enr = NULL;
		cr = NULL;
		syr = NULL;
		func_result_flag = 0;
	}
};

class NLPAnalysis{
private:
	static NLPAnalysis * nlpDinstance;
	NLPAnalysis();
	~NLPAnalysis();

public:
	void * segmentor_dics;			//�ִʴʵ�
	void * segmentor_entity_dics;	//ʵ��ʴʵ�
	void * classify_handle;			//����ʵ�
	void * sentence_handle;			//��ʽ�ļ�
	void * querytime_model;			//queryʱ�����ж�ģ��
	void * synonym_dics;			//ͬ��ʴʵ�
	unsigned int func_dic_flag;		//������Щ������Դ�����صı��

	/**
	 * Founction:instance
	 * 			���ص���ʵ��
	 * 	return: NLPAnalysis����Ψһ���
	 */
	static NLPAnalysis* instance();
	/**
	 * Founction:releaseInstance
	 * 			���ٵ���ʵ��
	 */
	static void releaseInstance();

	/**
	 * Founction:Init
	 * 			��ʼ�������شʵ�
	 * Parameter:
	 * 		flag [I] ��ʼ���ʵ���
	 * 		dictPath [I] �ʵ�·��
	 * 	return: bool
	 */
	bool Init(unsigned int flag, const char* dictPath);
	/**
	 * Founction:updateInit
	 * 			�ȼ���
	 * Parameter:
	 * 		dictPath [I] �ʵ�·��
	 * 	return: bool
	 */
	bool updateInit(const char* dictPath);

	/**
	 * Founction:CreateNLPAnalysisResult
	 * 			����NLPAnalysisInfo�ڴ�
	 * Parameter:
	 * 		flag [I] ��ʼ���ڴ�ռ���
	 * 		maxTermCount [I] ���ִ�term��Ŀ
	 * 		maxEntityCount [I] ���ʵ���term��Ŀ
	 * 	return: nlp����ṹ��
	 */
	NLPAnalysisInfo* CreateNLPAnalysisResult(unsigned int flag, int maxTermCount = 2048, int maxEntityCount = 512);
	/**
	 * Founction:DestroyNLPAnalysisResult
	 * 			����NLPAnalysisInfo�ڴ�
	 * Parameter:
	 * 		nlpAnalysisResult [I] nlp����ṹ��
	 * 	return: void
	 */
	void DestroyNLPAnalysisResult(NLPAnalysisInfo* nlpAnalysisResult);
	/**
	 * Founction:analysis
	 * 			nlp����������
	 * Parameter:
	 * 		segmentString [I] nlp����ṹ��
	 * 		result [I/O] nlp����ṹ��
	 * 		segmentLevels [I] �ִʵ����ȣ�Ĭ��Ϊ2
	 * 		clean_bad [I] ���������ʽʶ���в�����Ĭ��Ϊtrue
	 * 	return: bool
	 */
	bool analysis(const char* segmentString, NLPAnalysisInfo* result, unsigned int segmentLevels = 2, bool clean_bad = true);//������
	/**
	 * Founction:freeAll
	 * 			�ͷŴʵ���Դ
	 * 	return: bool
	 */
	bool freeAll();
	/**
	 * Founction:getEntityTypeInt
	 * 			ʵ������txt��uint��ת��
	 * Parameter:
	 * 		type [I] ʵ������ͣ��ʱ����ַ���
	 * 	return: int���͵�ʵ�������
	 */
	unsigned int getEntityTypeInt(char * type);
	/**
	 * Founction:getTermIDF
	 * 			��ѯidfֵ
	 * Parameter:
	 * 		input [I]:�ַ���
	 * 		len [I]  :�ַ�������
	 * return: int���͵�idfֵ��-1����ʾû�в�ѯ�����
	 */
	int getTermIDF(const char *input, unsigned int len);

private:
	void* 	segmentor_dics_bak[2];			//�ִʴʵ�
	void* 	segmentor_entity_dics_bak[2];	//ʵ��ʴʵ�
	void* 	classify_handle_bak[2];			//����ʵ�
	void* 	sentence_handle_bak[2];			//��ʽ�ļ�
	void* 	querytime_model_bak[2];			//queryʱ�����ж�ģ��
	void* 	synonym_dics_bak[2];			//ͬ��ʴʵ�
	int		n_initdic_tab;
	bool	loadBakDic(int order, const char* dictPath);			//���ر��ݴʵ�

	void 	NLP_process(const char* segmentString, NLPAnalysisInfo* result, unsigned int segmentLevels = 2, bool clean_bad = true);
	bool 	isSuckCat(int cat);
	bool 	isBadCat(int cat);
	bool 	isPerfectCat(int cat);
	bool 	isGoodCat(int cat);
	bool 	isLBSType(TypeInfoVec * cr);
	bool 	isLBSCat(int cat);
	bool 	isExtType(TypeInfoVec * cr);
	bool 	isExtCat(int cat);
	int 	ContextAdjust(const char* segmentString, EntityTermInfo* entt);
	bool	is_update;
};

};
#endif
