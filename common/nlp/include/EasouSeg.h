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

/********************************* �ִʽṹ���� *****************************************/

//�дʼ��������
#define SEGMENT_LEVEL_NUM		5
#define ENTITY_CATE_MAX_NUM		16

//�дʼ���(����)
enum SegmentLevel
{
	SEGMENT_LEVEL_BASIC  = 0,     //�������д�
	SEGMENT_LEVEL_MIXED  = 1,     //����д�
	SEGMENT_LEVEL_TERM   = 2,     //�����д�
	SEGMENT_LEVEL_PASTE1 = 3,     //ճ���д�1
	SEGMENT_LEVEL_PASTE2 = 4,     //ճ���д�2
};

#ifdef USE_POS
//���������б�
static const char* POS_STR[45] =
{	"Ag",   "Dg",  "Ng",  "Tg",  "Vg",  "a",   "ad",  "an", "b",  "c",
	"d",    "e",   "f",   "g",   "h",   "i",   "j",   "k",  "l",  "m",
	"n",    "nrc", "ns",  "nt",  "nx",  "nz",  "o",   "p",  "q",  "r",
	"s",    "t",   "u",   "v",   "vd",  "vn",  "w",   "y",  "z",  "ws",
	"****", "nr",  "nrf", "$",   "sh"};

//�������������б�
static const char* POS_CSTR[45] =
{	"������",   "������",     "������",  "ʱ����",   "������",   "���ݴ�",  "���δ�",   "���δ�",   "�����", "����",
	"����",     "̾��",       "��λ��",  "����",     "ǰ�ӳɷ�", "����",    "�������", "��ӳɷ�", "ϰ����", "����",
	"����",     "�ϳ�������", "����",    "��������", "��������", "����ר��", "������",   "���",    "����",   "����",
	"������",   "ʱ���",     "����",    "����",     "������",   "������",  "������", "������",   "״̬��", "δ֪��",
	"������",   "�ʵ�����",   "�����",  "����",     "����"};
#endif

//�д�term����Ϣ�ṹ
#pragma pack(1)
struct SegmentTermInfo
{
	int           offset:24;   //term��offset(�Ի�����Ϊ��λ)
	int           length:8;    //term�ĳ���
	unsigned char feature;     //term������
};

//�д�����
struct SegmentInput
{
	const char*   segmentString;  //�д��ַ���
	unsigned int  stringLength;   //�ַ�������
	unsigned int  segmentLevels;  //�ִ�level�ļ���
};

//�дʷ��صĽ��
struct SegmentResult
{
	SegmentTermInfo* termArray[SEGMENT_LEVEL_NUM];  //ָ���дʽ����termָ������
	unsigned int     termCount[SEGMENT_LEVEL_NUM];  //��termArray��Ӧ���дʽ����term�ĸ���
	void*            pInternalObject;               //�ڲ����󣬹��дʿⱣ��һЩ�м���Ϣ
};

//ʵ��ʺͻ�����term��Ϣ�ṹ
struct EntityTermInfo
{
	int           offset:24;   //term��offset(�ֽ�ƫ��)
	int           length:8;    //term��len(�ֽڳ���)
	char          *type_txt[ENTITY_CATE_MAX_NUM];  //term������,nt��ʾ������������������Ϊʵ��ʴʱ���
	unsigned int  type[ENTITY_CATE_MAX_NUM];      //�����͵�ʵ������
	char          hot[ENTITY_CATE_MAX_NUM];       //�ȶ�
	int           termInfo;    //ʵ��ʴʱ����ԣ�����Ϊ�������룬����Ĭ��Ϊ-1;
	bool          isKeep;      //��᪺��Ƿ���
};

//ʵ��ʺͻ�����ʶ����
struct EntityResult
{
	EntityTermInfo* entityInfo;         //ʶ������Ϣ
	int             count;              //ʵ��ʶ����ĸ���
	int             MaxSize;            //���ʶ�����
};

//�����ؼ�����Ϣ
struct keyWordInfo
{
	int term_index;  //mix term���
	float score;
	char cpos;   	//����
	int term_tf;  	//��Ƶ
	int len:8;  	//�ʳ�
	int idf:8;	//idf
	int offset;
	bool is_stop;	//ͣ�ôʺ�ͣ�ô���
};

#pragma pack()

/******************************** �ִʽӿ� *******************************************/

//���طִʿ⣬����Ҫ����һ��
//dicPath�����طִʿ���Ҫ��·��
//void*�����صķִʶ���ľ��
void* LoadSegmentLibrary(const char* dicPath, const char* addDicPath = NULL);

//ж�طִʿ⣬����Ҫ����һ��
bool UnloadSegmentLibrary(void* dicHandle);

//Ϊ�ִʽ�������ڴ�
//maxTermCount�����ܱ��зֵ����term��Ŀ,һ��ȡҪ���зֵ��ַ����ĳ���
SegmentResult* CreateSegmentResult(int maxTermCount = 2048);

//�дʽ��ʹ����Ϻ���Ҫ����
//segmentResult��Ҫ�����ٵ��дʶ���
void DestroySegmentResult(SegmentResult* segmentResult);

//���ַ��������д�
//dicHandle����Ҫʹ�õķִʶ�����
//input���ִʵ�����
//result���дʿ���Ҫ���Ľ��
//tagPOS���Ƿ���Ҫ���Ա�ע�������Ϊtrue���������ȷִʵ�ʱ�򣬻������Ⱥͻ�����Ȼ�����Ч�Ĵ�����Ϣ
bool WordSegment(void* dicHandle, SegmentInput &input, SegmentResult* result, bool tagPOS = false);

//���ַ��������д�
//result���дʿ���Ҫ���Ľ��
//segmentString���д��ַ���
//stringLength���ַ�������
//segmentLevels���ִ�level�ļ��ϣ�Ĭ�����е����ȶ���
//dicHandle����Ҫʹ�õķִʶ�����
//tagPOS���Ƿ���Ҫ���Ա�ע�������Ϊtrue���������ȷִʵ�ʱ�򣬻������Ⱥͻ�����Ȼ�����Ч�Ĵ�����Ϣ
bool WordSegment2(void* dicHandle,
				  SegmentResult*   result, 
				  const char*   segmentString, 
				  unsigned int  stringLength, 
				  unsigned int  segmentLevels = 4,
				  bool tagPOS = false);

//��ȡĳ�����ȷִʽ����term�ĸ���
//result���д����Ľ��
//segmentLevel���ִ�����
//����ֵ��ĳ�����ȷִʽ����term�ĸ���
unsigned int GetResultTermCount(SegmentResult* result, unsigned int segmentLevel);

//��ȡĳ�����ȷִʽ����ĳ��������term����
//result���д����Ľ��
//segmentLevel���ִ�����
//index���ִʽ��SegmentTermInfo������±�����
//����ֵ��ĳ�����ȷִʽ������Ӧ������term����
SegmentTermInfo* GetResultTermInfo(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//��ȡĳ��term���ַ�����ʾ
//result���д����Ľ��
//segmentLevel���ִ�����
//index���ִʽ��SegmentTermInfo������±�����
//dest��Ҫ����term �ַ����ı���
//destMaxLength��dest����󳤶�
//����ֵ��ָ��dest��ָ��
const char* GetTermString(SegmentResult* result, unsigned int segmentLevel, unsigned int index, char* dest, unsigned int destMaxLength);

//��ȡ�����ĳ��term��offset
//result���д����Ľ��
//segmentLevel���ִ�����
//index���ִʽ��SegmentTermInfo������±�����
//����ֵ����term��offset
//����ֵ˵�����ִ�����Ϊ0������termƫ�ƣ��ִ�����Ϊ1�������ϣ���������0��term index
unsigned int GetTermOffset(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//��ȡ�����ĳ��term�ĳ���
//result���д����Ľ��
//segmentLevel���ִ�����
//index���ִʽ��SegmentTermInfo������±�����
//����ֵ����term���ֽڳ���
unsigned int GetTermLength(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//��ȡ�����ĳ��term������������term�ĸ���
//result���д����Ľ��
//segmentLevel���ִ�����
//index���ִʽ��SegmentTermInfo������±�����
//����ֵ���ִ�����Ϊ0������1���ִ�����Ϊ1�������ϣ����غ�����0��term�ĸ���
unsigned int GetTermNumber(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//��ȡ�����ĳ��term������
//result���д����Ľ��
//segmentLevel���ִ�����
//index���ִʽ��SegmentTermInfo������±�����
//����ֵ����term������
unsigned char GetTermFeature(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

//��ȡ�����ĳ��term���������ϸ����term��index��Ӧ��ϵ
//result���д����Ľ��
//segmentLevel���ִ�����
//index���ִʽ��SegmentTermInfo������±�����
//����ֵ����term���ֽ�ƫ��
unsigned int GetTermByteOffset(SegmentResult* result, unsigned int segmentLevel, unsigned int index);

/******************************** ʵ���ʶ��ӿ� *******************************************/
//����ʵ��ʵ�
//entityDicPath������ʵ��ʴʵ���Ҫ��·��
//void*�����ص�ʵ��ʴʵ����ľ��
void *LoadEntityLibrary(const char* entityDicPath);

//ж��ʵ��ʵ�
//dicHandle��ʵ��ʴʵ����ľ��
bool UnloadEntityLibrary(void* dicHandle);

//����ʵ��ռ�
//maxTermCount������ʶ���term������һ��ȡҪ���зֵ��ַ������ַ�����
EntityResult* CreateEntityResult(int maxTermCount = 512);

//�ͷ�ʵ���ڴ�
//entityresult, ʵ��ʺͻ���������洢�ṹ��
void DestroyEntityResult(EntityResult *entityresult);

//ʵ��ʺͻ�����ʶ��
//result���ִʵ��дʽ��
//entityresult, ʵ��ʺͻ���������洢�ṹ��
//EntityDic�� ʵ�����ʵ�����ʵ���ʶ����; ���Ϊ�գ�ֻ���л�����ʶ��
//dicHandle����һ�ʵ䣬 ������ʶ����
//����ֵ��ʶ�������ʵ��ʺͻ���������������-1��ʾʶ��ʧ��
unsigned int EntityDectect(SegmentResult* result, EntityResult *entityresult, void* EntityDic=NULL);

//��ȡ��indexʵ�������߻��������ַ���
//result���ִʵ��дʽ��
//entityresult, ʵ��ʺͻ���������洢�ṹ��
//index�� ʶ�����ʵ�������߻��������
//dest��Ҫ����ʵ�������߻��������ַ�������
//destMaxLength��dest����󳤶�
//����ֵ��ָ��dest��ָ��
const char* GetEntityString(SegmentResult* result, EntityResult *entityresult, unsigned int index, char* dest, unsigned int destMaxLength);

//��ȡ��indexʵ�������߻��������ֽ�ƫ��
//result���ִʵ��дʽ��
//entityresult, ʵ��ʺͻ���������洢�ṹ��
//index��ʶ�����ʵ�������߻��������
//����ֵ����ʵ�������߻��������ֽ�ƫ��
unsigned int GetEntityByteOffset(EntityResult *entityresult, unsigned int index);

//��ȡ��indexʵ�������߻��������ֽڳ���
//result���ִʵ��дʽ��
//entityresult, ʵ��ʺͻ���������洢�ṹ��
//index��ʶ�����ʵ�������߻��������
//����ֵ����ʵ�������߻��������ֽڳ���
unsigned int GetEntityLength(EntityResult *entityresult, unsigned int index);

/******************************** �ؼ�����ȡ�ӿ� *******************************************/
//��ȡ�ִ��ַ����Ĺؼ���(���8����)
//dicHandle����һ�ʵ䣬 �ؼ�����ȡ���ôʵ���Ϣ
//result���ִ��дʽ��
//keywords����ȡ�Ĺؼ��ʽ��
//����ֵ������ɹ����عؼ��ʸ���0-8������ʧ�ܷ���-1.
int GetKeyWords(void* dicHandle, SegmentResult* result,std::vector<std::string> &keywords);
int GetKeyWordsInfo(void* dicHandle, SegmentResult* result, std::vector<keyWordInfo>& keyWordInfo_array);

/** �ж�ʵ���ڻ���������Ƿ���ȻΪʵ��
 *
 *result���ִʽ��
 *pos��λ��by byte
 *length������by byte
 *����ֵ��true�����������ʵ�壬false���ǻ��������ʵ��
 */
bool isMixTerm(SegmentResult* result, int pos, int length);

/******************************** idf��ѯ�ӿ� *******************************************/
//���ַ��������д�
//dicHandle����Ҫʹ�õķִʶ�����
//input��term���ַ���
//len���ַ�������
//����ֵ��-1��û�в�ѯ���������������idf��ֵ
int getIDF(void* dicHandle, const char *input, unsigned int len);

#endif /* _EASOU_SEG_H */

