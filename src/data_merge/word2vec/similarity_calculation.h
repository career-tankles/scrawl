/*
 * similarity_calculation.h
 *
 *  Created on: 2013-11-7
 *      Author: xunwu
 */

#ifndef SIMILARITY_CALCULATION_H_
#define SIMILARITY_CALCULATION_H_

#include <stdint.h>
#include <map>

#include "NLPAnalysis.h" //easou分词接口定义

using namespace std;
using namespace analysis;


//#define MAX_TEXT_NUM 1024*1024*20
//#define MAX_TERM_NUM	1024*1024*5
#define MAX_TEXT_NUM 1024*20
#define MAX_TERM_NUM	1024*5
#define MAX_LINE_LEN 1024
#define MAX_TEXT_LEN 128
#define TERM_VEC_LEN	200
#define MAX_TERM_LEN	30
#define MAX_TERMS_COUNT	20



#define BASE_TERM_HASHENTRY_SIZE            2000000
#define BASE_TERM_MEMBLOCK_COUNT            1024
#define BASE_TERM_MEMBLOCK_SIZE             (1024*1024*1)

struct term_vec_model_t{
	uint64_t term_count;
	uint64_t vec_size;
	void *term_vec_hashmap;
};

struct short_text_t{
	char text_str[MAX_TEXT_LEN];
	uint8_t	term_count;
	char term_str[MAX_TERMS_COUNT][MAX_TERM_LEN];
	uint64_t	term_sign[MAX_TERMS_COUNT];
	uint8_t especial_count;
	uint16_t	especial_featrue[MAX_TERMS_COUNT];
//	float text_vec[TERM_VEC_LEN];
//	int	entity_count;
//	int	entity_sign[MAX_TERMS_COUNT];
//	float entity_vec[TERM_VEC_LEN];
};

struct short_text_pool_t{
	int	text_count;
	short_text_t *short_text;
};

//vocab, term_vec需要释放内存
int init_term_vec_model(FILE* input_file, uint64_t& term_count, char* vocab, float *term_vec);

int get_text_vec(NLPAnalysis* analysis, NLPAnalysisInfo* result,
		term_vec_model_t *term_vec_model, short_text_t* short_text);

int init_short_text_pool(NLPAnalysis* analysis, NLPAnalysisInfo* result, FILE* input_file,
		term_vec_model_t *term_vec_model, short_text_pool_t* short_text_pool);

int get_similarity_text(NLPAnalysis* analysis, NLPAnalysisInfo* result,
		char* text, char* similarity_text,  float *similarity_dis,
		short_text_pool_t* short_text_pool, term_vec_model_t *term_vec_model);

struct word2vec_s 
{
    FILE* model_file_fd;
    term_vec_model_t term_vec_model;

	NLPAnalysis* analysis;
	NLPAnalysisInfo* result;
    word2vec_s(){
        model_file_fd = NULL;
        analysis = NULL;
        result = NULL;
    }
};
int word2vec_init(word2vec_s& s, const char* model_file, const char* dict);
void word2vec_destroy(word2vec_s& s);
int word2vec_calc(word2vec_s& s, std::string& txt1, std::string& txt2, double& sim);


#endif /* SIMILARITY_CALCULATION_H_ */
