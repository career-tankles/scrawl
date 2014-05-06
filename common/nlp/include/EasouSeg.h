/**
* @file EasouSeg.h
* @author Ryan, Beck, Sdctw
* @date Tue Jan 08 14:00:00 CST 2014
* @version 1.4.5
* @brief Easou Chinese word segmentation,need newDicmapv2.9.6.bin
*  
**/
#ifndef _EASOU_SEG_H
#define _EASOU_SEG_H

#include <string>
#include <vector>

/********************************* 分词结构定义 *****************************************/

//切词级别的数量
#define SEGMENT_LEVEL_NUM		5
#define ENTITY_CATE_MAX_NUM		16

//切词级别(粒度)
enum SegmentLevel
{
	SEGMENT_LEVEL_BASIC  = 0,     //基本词切词
	SEGMENT_LEVEL_MIXED  = 1,     //混合切词
	SEGMENT_LEVEL_TERM   = 2,     //词组切词
	SEGMENT_LEVEL_PASTE1 = 3,     //粘贴切词1
	SEGMENT_LEVEL_PASTE2 = 4,     //粘贴切词2
};

#ifdef USE_POS
//词性名称列表
static const char* POS_STR[45] =
{	"Ag",   "Dg",  "Ng",  "Tg",  "Vg",  "a",   "ad",  "an", "b",  "c",
	"d",    "e",   "f",   "g",   "h",   "i",   "j",   "k",  "l",  "m",
	"n",    "nrc", "ns",  "nt",  "nx",  "nz",  "o",   "p",  "q",  "r",
	"s",    "t",   "u",   "v",   "vd",  "vn",  "w",   "y",  "z",  "ws",
	"****", "nr",  "nrf", "$",   "sh"};

//词性中文名称列表
static const char* POS_CSTR[45] =
{	"形语素",   "副语素",     "名语素",  "时语素",   "动语素",   "形容词",  "副形词",   "名形词",   "区别词", "连词",
	"副词",     "叹词",       "方位词",  "语素",     "前接成分", "成语",    "简称略语", "后接成分", "习用语", "数词",
	"名词",     "合成中文名", "地名",    "机构团体", "非语素字", "其他专名", "拟声词",   "介词",    "量词",   "代词",
	"处所词",   "时间词",     "助词",    "动词",     "副动词",   "名动词",  "标点符号", "语气词",   "状态词", "未知词",
	"无意义",   "词典人名",   "外国名",  "货币",     "缩略"};
#endif

//切词term的信息结构
#pragma pack(1)
struct SegmentTermInfo
{
	int           offset:24;   //term的offset(以基本词为单位)
	int           length:8;    //term的长度
	unsigned char feature;     //term的属性
};

//切词输入
struct SegmentInput
{
	const char*   segmentString;  //切词字符串
	unsigned int  stringLength;   //字符串长度
	unsigned int  segmentLevels;  //分词level的集合
};

//切词返回的结果
struct SegmentResult
{
	SegmentTermInfo* termArray[SEGMENT_LEVEL_NUM];  //指向切词结果的term指针数组
	unsigned int     termCount[SEGMENT_LEVEL_NUM];  //与termArray对应的切词结果的term的个数
	void*            pInternalObject;               //内部对象，供切词库保存一些中间信息
};

//实体词和机构名term信息结构
struct EntityTermInfo
{
	int           offset:24;   //term的offset(字节偏移)
	int           length:8;    //term的len(字节长度)
	char          *type_txt[ENTITY_CATE_MAX_NUM];  //term的类型,nt表示机构名，其他的名字为实体词词表名
	unsigned int  type[ENTITY_CATE_MAX_NUM];      //数字型的实体类型
	char          hot[ENTITY_CATE_MAX_NUM];       //热度
	int           termInfo;    //实体词词表属性，地名为地名编码，其他默认为-1;
	bool          isKeep;      //消岐后是否保留
};

//实体词和机构名识别结果
struct EntityResult
{
	EntityTermInfo* entityInfo;         //识别结果信息
	int             count;              //实际识别出的个数
	int             MaxSize;            //最大识别个数
};

//后续关键词信息
struct keyWordInfo
{
	int term_index;  //mix term序号
	float score;
	char cpos;   	//词性
	int term_tf;  	//词频
	int len:8;  	//词长
	int idf:8;	//idf
	int offset;
	bool is_stop;	//停用词和停用词性
};

#pragma pack()

/******************************** 分词接口 *******************************************/

//加载分词库，仅需要调用一次
//dicPath，加载分词库需要的路径
//void*，返回的分词对象的句柄
void* LoadSegmentLibrary(const char* dicPath, const char* addDicPath = NULL);

//卸载分词库，仅需要调用一次
bool UnloadSegmentLibrary(void* dicHandle);

//为分词结果分配内存
//maxTermCount，可能被切分的最大term数目,一般取要被切分的字符串的长度
SegmentResult* CreateSegmentResult(int maxTermCount = 2048);

//切词结果使用完毕后需要销毁
//segmentResult，要被销毁的切词对象
void DestroySegmentResult(SegmentResult* segmentResult);

//对字符串进行切词
//dicHandle，需要使用的分词对象句柄
//input，分词的输入
//result，切词库需要填充的结果
//tagPOS，是否需要词性标注，如果设为true，当多粒度分词的时候，基本粒度和混合粒度会有有效的词性信息
bool WordSegment(void* dicHandle, SegmentInput &input, SegmentResult* result, bool tagPOS = false);

//对字符串进行切词
//result，切词库需要填充的结果
//segmentString，切词字符串
//stringLength，字符串长度
//segmentLevels，分词level的集合，默认所有的粒度都切
//dicHandle，需要使用的分词对象句柄
//tagPOS，是否需要词性标注，如果设为true，当多粒度分词的时候，基本粒度和混合粒度会有有效的词性信息
bool WordSegment2(void* dicHandle,
				  SegmentResult*   result, 
				  const char*   segmentString, 
				  unsigned int  stringLength, 
				  unsigned int  segmentLevels = 4,
				  bool tagPOS = false);

//获取某种粒度分词结果的term的个数
//result，切词填充的结果
//segmentLevel，分词粒度
//返回值，某种粒度分词结果的term的个数
unsigned int GetResultTermCount(SegmentResult* result, unsigned int segmentLevel);

//获取某种粒度分词结果的某个索引的term属性
//result，切词填充的结果
//segmentLevel，分词粒度
//index，分词结果SegmentTermInfo数组的下标索引
//返回值，某种粒度分词结果的相应索引的term属性
SegmentTermInfo* GetResultTermInfo(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//获取某个term的字符串表示
//result，切词填充的结果
//segmentLevel，分词粒度
//index，分词结果SegmentTermInfo数组的下标索引
//dest，要保存term 字符串的变量
//destMaxLength，dest的最大长度
//返回值，指向dest的指针
const char* GetTermString(SegmentResult* result, unsigned int segmentLevel, unsigned int index, char* dest, unsigned int destMaxLength);

//获取结果中某个term的offset
//result，切词填充的结果
//segmentLevel，分词粒度
//index，分词结果SegmentTermInfo数组的下标索引
//返回值，此term的offset
//返回值说明：分词粒度为0，返回term偏移；分词粒度为1或者以上，返回粒度0的term index
unsigned int GetTermOffset(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//获取结果中某个term的长度
//result，切词填充的结果
//segmentLevel，分词粒度
//index，分词结果SegmentTermInfo数组的下标索引
//返回值，此term的字节长度
unsigned int GetTermLength(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//获取结果中某个term含基本粒度下term的个数
//result，切词填充的结果
//segmentLevel，分词粒度
//index，分词结果SegmentTermInfo数组的下标索引
//返回值，分词粒度为0，返回1；分词粒度为1或者以上，返回含粒度0的term的个数
unsigned int GetTermNumber(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//获取结果中某个term的属性
//result，切词填充的结果
//segmentLevel，分词粒度
//index，分词结果SegmentTermInfo数组的下标索引
//返回值，此term的属性
unsigned char GetTermFeature(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//获取结果中某个term的相对于最细粒度term的index对应关系
//result，切词填充的结果
//segmentLevel，分词粒度
//index，分词结果SegmentTermInfo数组的下标索引
//返回值，此term的字节偏移
unsigned int GetTermByteOffset(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

/******************************** 实体词识别接口 *******************************************/
//加载实体词典
//entityDicPath，加载实体词词典需要的路径
//void*，返回的实体词词典对象的句柄
void *LoadEntityLibrary(const char* entityDicPath);

//卸载实体词典
//dicHandle，实体词词典对象的句柄
bool UnloadEntityLibrary(void* dicHandle);

//分配实体空间
//maxTermCount，可能识别的term个数，一般取要被切分的字符串的字符长度
EntityResult* CreateEntityResult(int maxTermCount = 512);

//释放实体内存
//entityresult, 实体词和机构名结果存储结构体
void DestroyEntityResult(EntityResult *entityresult);

//实体词和机构名识别
//result，分词的切词结果
//entityresult, 实体词和机构名结果存储结构体
//EntityDic， 实体名词典句柄，实体词识别用; 如果为空，只进行机构名识别
//dicHandle，第一词典， 机构名识别用
//返回值，识别出来的实体词和机构名个数，等于-1表示识别失败
unsigned int EntityDectect(SegmentResult* result, EntityResult *entityresult, void* EntityDic=NULL);

//获取第index实体名或者机构名的字符串
//result，分词的切词结果
//entityresult, 实体词和机构名结果存储结构体
//index， 识别出的实体名或者机构名序号
//dest，要保存实体名或者机构名的字符串变量
//destMaxLength，dest的最大长度
//返回值，指向dest的指针
const char* GetEntityString(SegmentResult* result, EntityResult *entityresult, unsigned int index, char* dest, unsigned int destMaxLength);

//获取第index实体名或者机构名的字节偏移
//result，分词的切词结果
//entityresult, 实体词和机构名结果存储结构体
//index，识别出的实体名或者机构名序号
//返回值，该实体名或者机构名的字节偏移
unsigned int GetEntityByteOffset(EntityResult *entityresult, unsigned int index);

//获取第index实体名或者机构名的字节长度
//result，分词的切词结果
//entityresult, 实体词和机构名结果存储结构体
//index，识别出的实体名或者机构名序号
//返回值，该实体名或者机构名的字节长度
unsigned int GetEntityLength(EntityResult *entityresult, unsigned int index);

/******************************** 关键词提取接口 *******************************************/
//获取分词字符串的关键词(最多8个词)
//dicHandle，第一词典， 关键词提取利用词典信息
//result，分词切词结果
//keywords，获取的关键词结果
//返回值，运算成功返回关键词个数0-8，运算失败返回-1.
int GetKeyWords(void* dicHandle, SegmentResult* result,std::vector<std::string> &keywords);
int GetKeyWordsInfo(void* dicHandle, SegmentResult* result, std::vector<keyWordInfo>& keyWordInfo_array);

/** 判断实体在混合粒度下是否依然为实体
 *
 *result，分词结果
 *pos，位置by byte
 *length，长度by byte
 *返回值，true：混合粒度下实体，false，非混合粒度下实体
 */
bool isMixTerm(SegmentResult* result, int pos, int length);

/******************************** idf查询接口 *******************************************/
//对字符串进行切词
//dicHandle，需要使用的分词对象句柄
//input，term的字符串
//len，字符串长度
//返回值，-1：没有查询到结果，返回正常idf数值
int getIDF(void* dicHandle, const char *input, unsigned int len);

#endif /* _EASOU_SEG_H */

