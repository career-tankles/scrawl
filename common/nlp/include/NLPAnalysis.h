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

//初始化词典和内存空间的标记，一般情况下，两个标记设为一致
#define SEGMENT_FLAG		1	//segword and entity
#define CLASSIFY_FLAG		2	//class
#define ONLY_SEGMENT_FLAG	4	//only segword
#define SYNONYM_FLAG		8	//synonym
#define QUERY_ENTITY_FLAG 16 //query entity recognition
#define NATURE_ENTITY_FLAG 32 //nature text entity recognition
//nlp分析结果结构体
struct NLPAnalysisInfo{
	SegmentResult*	sr;		//segment result
	EntityResult*	enr;	//entity result
	TypeInfoVec*	cr;		//classify result, sentence type inside
	syn_result*		syr;	//synonym result
	unsigned int 	func_result_flag;	//设置哪些功能空间被初始化的标记
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
	void * segmentor_dics;			//分词词典
	void * segmentor_entity_dics;	//实体词词典
	void * classify_handle;			//分类词典
	void * sentence_handle;			//句式文件
	void * querytime_model;			//query时间敏感度模型
	void * synonym_dics;			//同义词词典
	unsigned int func_dic_flag;		//设置哪些功能资源被加载的标记

	/**
	 * Founction:instance
	 * 			返回单体实例
	 * 	return: NLPAnalysis对象唯一句柄
	 */
	static NLPAnalysis* instance();
	/**
	 * Founction:releaseInstance
	 * 			销毁单体实例
	 */
	static void releaseInstance();

	/**
	 * Founction:Init
	 * 			初始化，加载词典
	 * Parameter:
	 * 		flag [I] 初始化词典标记
	 * 		dictPath [I] 词典路径
	 * 	return: bool
	 */
	bool Init(unsigned int flag, const char* dictPath);
	/**
	 * Founction:updateInit
	 * 			热加载
	 * Parameter:
	 * 		dictPath [I] 词典路径
	 * 	return: bool
	 */
	bool updateInit(const char* dictPath);

	/**
	 * Founction:CreateNLPAnalysisResult
	 * 			创建NLPAnalysisInfo内存
	 * Parameter:
	 * 		flag [I] 初始化内存空间标记
	 * 		maxTermCount [I] 最大分词term数目
	 * 		maxEntityCount [I] 最大实体词term数目
	 * 	return: nlp结果结构体
	 */
	NLPAnalysisInfo* CreateNLPAnalysisResult(unsigned int flag, int maxTermCount = 2048, int maxEntityCount = 512);
	/**
	 * Founction:DestroyNLPAnalysisResult
	 * 			销毁NLPAnalysisInfo内存
	 * Parameter:
	 * 		nlpAnalysisResult [I] nlp结果结构体
	 * 	return: void
	 */
	void DestroyNLPAnalysisResult(NLPAnalysisInfo* nlpAnalysisResult);
	/**
	 * Founction:analysis
	 * 			nlp处理主函数
	 * Parameter:
	 * 		segmentString [I] nlp结果结构体
	 * 		result [I/O] nlp结果结构体
	 * 		segmentLevels [I] 分词的粒度，默认为2
	 * 		clean_bad [I] 二级分类句式识别中参数，默认为true
	 * 	return: bool
	 */
	bool analysis(const char* segmentString, NLPAnalysisInfo* result, unsigned int segmentLevels = 2, bool clean_bad = true);//主函数
	/**
	 * Founction:freeAll
	 * 			释放词典资源
	 * 	return: bool
	 */
	bool freeAll();
	/**
	 * Founction:getEntityTypeInt
	 * 			实体类型txt到uint的转换
	 * Parameter:
	 * 		type [I] 实体词类型：词表名字符串
	 * 	return: int类型的实体词类型
	 */
	unsigned int getEntityTypeInt(char * type);
	/**
	 * Founction:getTermIDF
	 * 			查询idf值
	 * Parameter:
	 * 		input [I]:字符串
	 * 		len [I]  :字符串长度
	 * return: int类型的idf值，-1，表示没有查询到结果
	 */
	int getTermIDF(const char *input, unsigned int len);

private:
	void* 	segmentor_dics_bak[2];			//分词词典
	void* 	segmentor_entity_dics_bak[2];	//实体词词典
	void* 	classify_handle_bak[2];			//分类词典
	void* 	sentence_handle_bak[2];			//句式文件
	void* 	querytime_model_bak[2];			//query时间敏感度模型
	void* 	synonym_dics_bak[2];			//同义词词典
	int		n_initdic_tab;
	bool	loadBakDic(int order, const char* dictPath);			//加载备份词典

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
