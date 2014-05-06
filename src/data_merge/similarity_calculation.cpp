/*
 * similarity_calculation.cpp
 *
 *  Created on: 2013-11-7
 *      Author: xunwu
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "sign.h"
#include "ctool_hashmap.h"
//#include "hashmap.h"
#include "similarity_calculation.h"

#define MAX_SIMILARITY_NUM            20

/* 200w hash, 1M memory block, 1G total memory. */
#define QS_BASE_HMAP3_HASHENTRY_SIZE            2000000
#define QS_BASE_HMAP3_MEMBLOCK_COUNT            1024
#define QS_BASE_HMAP3_MEMBLOCK_SIZE             (1024*1024*1)


uint64_t sign64MD5Ex(const char* str_ptr, uint32_t str_len) {
	uint64_t sign;

	if (str_len <= 8) {
		sign = 0;
		memcpy(&sign, str_ptr, str_len);
	} else {
		EA_COMMON::GetSign64MD5(str_ptr, str_len, sign);
	}
	return sign;
}


int init_term_vec_model(FILE* input_file, term_vec_model_t *term_vec_model){
	char term[MAX_TERM_LEN];
	float term_vec[TERM_VEC_LEN];
	char ch;

	uint64_t pos = 0;

	fscanf(input_file, "%lu", &term_vec_model->term_count);
	fscanf(input_file, "%lu", &term_vec_model->vec_size);

	for (int i = 0; i < term_vec_model->term_count; i++) {
		fscanf(input_file, "%s%c", term, &ch);
		uint64_t term_sign = sign64MD5Ex(term, strlen(term));
		fread(term_vec, sizeof(float), term_vec_model->vec_size, input_file);
		void *buf = ctool_hashmap_buf(term_vec_model->term_vec_hashmap, term_vec_model->vec_size * sizeof(float));
		if(buf == NULL){
			fprintf(stderr, "hash map memory overflow!\n");
			continue;
		}
		memcpy(buf, term_vec, term_vec_model->vec_size*sizeof(float));
		ctool_hashmap_add(term_vec_model->term_vec_hashmap, term_sign, buf);
	  }
	return 0;
}

int get_short_text(NLPAnalysis* analysis, NLPAnalysisInfo* result,
		term_vec_model_t *term_vec_model, short_text_t* short_text){
	SegmentResult* seg_result;
	EntityResult* entity_result;

	uint8_t term_featrue;
	int term_count;
	int term_offset;
	int	term_len;
	uint64_t term_sign;

	int entity_count;
	int entity_start_offset;
	int entity_end_offset;
	int entity_len;

	float vec_len;
	float entity_vec_len;

	int i, j, k;

	short_text->term_count = 0;
	short_text->especial_count = 0;

	analysis->analysis(short_text->text_str, result, 1, true);

	seg_result = result->sr;
	entity_result = result->enr;
	term_count = seg_result->termCount[1];
	entity_count = entity_result->count;

	for(int i = 0, j = 0; i < term_count && i < MAX_TERMS_COUNT; i++){

		if(seg_result->termArray[1][i].feature == 24){
			break;
		}

		if(seg_result->termArray[1][i].feature == 36){
			continue;
		}

		term_offset = GetTermByteOffset(seg_result, 1, i);
		if(i == term_count-1){
			term_len = strlen(short_text->text_str)-term_offset;
		}
		else{
			term_len = GetTermByteOffset(seg_result, 1, i+1)-term_offset;
		}
		entity_start_offset = 0;
		entity_len = 0;
		entity_end_offset = 0;
		bool ctm = false;
		for(int k = 0; k < 8; k++){
			if(entity_result->entityInfo[j].type[k] < 1000 &&
					entity_result->entityInfo[j].type[k] > 0){
//				entity_result->entityInfo[j].type[0] = entity_result->entityInfo[j].type[k];
				ctm = true;
				break;
			}
		}
		if(entity_count > j && entity_result->entityInfo[j].isKeep && ctm){
			entity_start_offset = entity_result->entityInfo[j].offset;
			entity_len = entity_result->entityInfo[j].length;
			entity_end_offset = entity_start_offset+entity_len-1;
		}
		if(entity_end_offset < term_offset){
			j++;
			ctm = false;
			for(int k = 0; k < 8; k++){
				if(entity_result->entityInfo[j].type[k] < 1000 &&
						entity_result->entityInfo[j].type[k] > 0){
					ctm = true;
					break;
				}
			}
			if(entity_count > j && entity_result->entityInfo[j].isKeep && ctm){
				entity_start_offset = entity_result->entityInfo[j].offset;
				entity_len = entity_result->entityInfo[j].length;
				entity_end_offset = entity_start_offset+entity_len-1;
			}
			else{
				entity_start_offset = 65535;
				entity_len = 0;
				entity_end_offset = 65535;
			}
		}
		if(entity_start_offset > term_offset){
			term_sign = sign64MD5Ex(short_text->text_str+term_offset, term_len);
			short_text->term_sign[short_text->term_count] = term_sign;
			GetTermString(seg_result, 1, i, short_text->term_str[short_text->term_count], MAX_TERM_LEN);
			term_featrue = GetTermFeature(seg_result, 1, i);
			if((term_featrue > 20 && term_featrue < 26) ||
					(term_featrue > 40 && term_featrue < 45) ||
					term_featrue == 19 || term_featrue == 28 ||
					term_featrue == 31){
				short_text->especial_featrue[short_text->term_count] = term_featrue;
				short_text->especial_count++;
			}
			else{
				short_text->especial_featrue[short_text->term_count] = 0;
			}

			short_text->term_count++;
		}
		else if(entity_start_offset == term_offset){
			if(term_len > entity_len){
				term_sign = sign64MD5Ex(short_text->text_str+term_offset, term_len);
				short_text->term_sign[short_text->term_count] = term_sign;
				GetTermString(seg_result, 1, i, short_text->term_str[short_text->term_count], MAX_TERM_LEN);
				term_featrue = GetTermFeature(seg_result, 1, i);
				if((term_featrue > 20 && term_featrue < 26) ||
						(term_featrue > 40 && term_featrue < 45) ||
						term_featrue == 19 || term_featrue == 28 ||
						term_featrue == 31){
					short_text->especial_featrue[short_text->term_count] = term_featrue;
					short_text->especial_count++;
				}
				else{
					short_text->especial_featrue[short_text->term_count] = 0;
				}

				short_text->term_count++;
			}
			else{
				term_sign = sign64MD5Ex(short_text->text_str+entity_start_offset, entity_len);
				short_text->term_sign[short_text->term_count] = term_sign;
				GetEntityString(seg_result, entity_result, j, short_text->term_str[short_text->term_count], MAX_TERM_LEN);

				for(int k = 0; k < 8; k++){
					if(entity_result->entityInfo[j].type[k] < 1000){
						short_text->especial_featrue[short_text->term_count] =
								entity_result->entityInfo[j].type[k]+1000;
						break;
					}
				}

				short_text->especial_count++;
				short_text->term_count++;

			}
		}
		else if(entity_end_offset >= term_offset){
			continue;
		}
		else if(entity_end_offset < term_offset){
			term_sign = sign64MD5Ex(short_text->text_str+term_offset, term_len);
			short_text->term_sign[short_text->term_count] = term_sign;
			GetTermString(seg_result, 1, i, short_text->term_str[short_text->term_count], MAX_TERM_LEN);
			term_featrue = GetTermFeature(seg_result, 1, i);
			if((term_featrue > 20 && term_featrue < 26) ||
					(term_featrue > 40 && term_featrue < 45) ||
					term_featrue == 19 || term_featrue == 28 ||
					term_featrue == 31){
				short_text->especial_featrue[short_text->term_count] = term_featrue;
				short_text->especial_count++;
			}
			else{
				short_text->especial_featrue[short_text->term_count] = 0;
			}

			short_text->term_count++;
		}
	}

	if(short_text->term_count < 2){
		return -1;
	}

	return 0;
}


int init_short_text_pool(NLPAnalysis* analysis, NLPAnalysisInfo* result, FILE* input_file,
		term_vec_model_t *term_vec_model, short_text_pool_t* short_text_pool){

	SegmentResult* seg_result;
	EntityResult* entity_result;

	int term_count;
	int term_offset;
	int	term_len;

	int entity_count;
	int entity_start_offset;
	int entity_end_offset;
	int entity_len;

	char line_buf[MAX_LINE_LEN] = {0};

	char *text = NULL;
	char *frequency = NULL;
	int query_frequency = 0;

	uint64_t term_sign = 0;

	int text_num = 0;

	int i, j;

	while (fgets(line_buf, 1024, input_file)) {
		text = line_buf;
		while (*text == '\t' || *text == ' ') {
			text++;
		}

		while (text[strlen(text)-1]=='\r'||text[strlen(text)-1]=='\n') {
			text[strlen(text)-1]='\0';
		}
		if(strlen(text) == 0){
			continue;
		}
		frequency = text;
		while(*text != '\t' && *text != ' '){
			text++;
		}
		*text = '\0';
		text++;
		if(strlen(frequency) > 8 || *frequency < 48 || *frequency > 57){
			continue;
		}

		if(strlen(text) >= MAX_TEXT_LEN || strlen(text) == 0){
			continue;
		}

//		fprintf(stderr, "text_num:%d\n", text_num);
		strncpy(short_text_pool->short_text[text_num].text_str, text, strlen(text));
		short_text_pool->short_text[text_num].text_str[strlen(text)] = 0;
		if(get_short_text(analysis, result, term_vec_model, &short_text_pool->short_text[text_num]) == 0){
			text_num++;
		}

		if(text_num%10000 == 0){
			fprintf(stderr, "line_num:%d\n", text_num);
		}
		if(text_num >= MAX_TEXT_NUM){
			break;
		}
	}
	short_text_pool->text_count = text_num;
	return 0;
}

void word2vec_destroy(word2vec_s& s)
{
	if(s.model_file_fd){
		fclose(s.model_file_fd);
		s.model_file_fd = 0;
	}

	if(s.term_vec_model.term_vec_hashmap){
		ctool_hashmap_drop(s.term_vec_model.term_vec_hashmap);
	}

	if(s.result && s.analysis){
		s.analysis->DestroyNLPAnalysisResult(s.result);
		s.result = 0;
	}

	if(s.analysis){
		s.analysis->freeAll();
		s.analysis = 0;
		NLPAnalysis::releaseInstance();
	}
}


int word2vec_init(word2vec_s& s, const char* model_file, const char* dict)
{
	s.model_file_fd = fopen(model_file, "rb");
	if (s.model_file_fd == NULL) {
	    fprintf(stderr, "model file not found\n");
        word2vec_destroy(s);
	}

    try{
    	s.analysis = NLPAnalysis::instance();
        if(s.analysis && s.analysis->Init(1, dict)){
        	fprintf(stderr, "load nlp library ok\n");
        }
        else {
        	fprintf(stderr, "load nlp library fail\n");
            word2vec_destroy(s);
        }

        s.result = s.analysis->CreateNLPAnalysisResult(1);
        if(s.result){
            fprintf(stderr, "create nlp s.result ok\n");
        }
        else{
        	fprintf(stderr, "create nlp s.result fail\n");
            word2vec_destroy(s);
        }

    } catch(...){
    	fprintf(stderr, "load nlp library error %s\n", dict);
        word2vec_destroy(s);
    }

    s.term_vec_model.term_vec_hashmap =
    		ctool_hashmap_init(BASE_TERM_HASHENTRY_SIZE, BASE_TERM_MEMBLOCK_COUNT, BASE_TERM_MEMBLOCK_SIZE);
	init_term_vec_model(s.model_file_fd, &s.term_vec_model);

    return 0;
}


int word2vec_calc(word2vec_s& s, std::string& txt1, std::string& txt2, double& sim)
{
	short_text_t first_text;
	short_text_t second_text;

	char *flag;

	float *term_vec;
	float first_vec[TERM_VEC_LEN];
	float second_vec[TERM_VEC_LEN];

	uint64_t first_sign[MAX_TERMS_COUNT];
	uint64_t second_sign[MAX_TERMS_COUNT];

	uint8_t first_especial_count;
	uint8_t second_especial_count;

	float vec_len;

	float text_dist = 0;
	float threshold = 0;

//
	char text_str[1024];

	    	strncpy(first_text.text_str, txt1.c_str(), txt1.size());
	    	first_text.text_str[strlen(first_text.text_str)] = 0;

	    	strncpy(second_text.text_str, txt2.c_str(), txt2.size());
	    	second_text.text_str[strlen(second_text.text_str)] = 0;

    		get_short_text(s.analysis, s.result, &(s.term_vec_model), &first_text);
    		get_short_text(s.analysis, s.result, &(s.term_vec_model), &second_text);

    		for(int j = 0; j < s.term_vec_model.vec_size; j++){
    			first_vec[j] = 0;
    			second_vec[j] = 0;
    		}

            fprintf(stderr, "first_text.term_count=%d second_text.term_count=%d\n", first_text.term_count, second_text.term_count);

    		if(first_text.term_count > 1 && second_text.term_count > 1){
                int term_count = first_text.term_count + second_text.term_count;
	    		threshold = 0.88+term_count*0.015;
	    		if(threshold > 0.97){
	    			threshold = 0.97;
	    		}

	    		first_especial_count = first_text.especial_count;
	    		second_especial_count = second_text.especial_count;

	    		for(int j = 0; j < first_text.term_count; j++){
	    			first_sign[j] = first_text.term_sign[j];
	    		}

    			for(int j = 0; j < first_text.term_count; j++){
    				term_vec = (float*) ctool_hashmap_get(s.term_vec_model.term_vec_hashmap, first_sign[j], NULL);
					if(term_vec){
						for(int k = 0; k < s.term_vec_model.vec_size; k++){
							first_vec[k] += term_vec[k];
						}
					}
    			}

				vec_len = 0;
				for (int j = 0; j < s.term_vec_model.vec_size; j++) {
					vec_len += first_vec[j] * first_vec[j];
				}
				vec_len = sqrt(vec_len);
				for (int j = 0; j < s.term_vec_model.vec_size; j++) {
					first_vec[j] /= vec_len;
				}


    			for(int k = 0; k < second_text.term_count; k++){
    				second_sign[k] = second_text.term_sign[k];
    			}

    			for(int k = 0; k < second_text.term_count; k++){
    				term_vec = (float*) ctool_hashmap_get(s.term_vec_model.term_vec_hashmap, second_sign[k], NULL);
					if(term_vec){
						for(int t = 0; t < s.term_vec_model.vec_size; t++){
							second_vec[t] += term_vec[t];
						}
					}
    			}
				vec_len = 0;
				for (int k = 0; k < s.term_vec_model.vec_size; k++) {
					vec_len += second_vec[k] * second_vec[k];
				}
				vec_len = sqrt(vec_len);
				for (int k = 0; k < s.term_vec_model.vec_size; k++) {
					second_vec[k] /= vec_len;
				}

    			text_dist = 0;

				for(int k = 0; k < s.term_vec_model.vec_size; k++){
					text_dist += first_vec[k] * second_vec[k];
				}

	    		if(first_especial_count != second_especial_count){
	    			text_dist /= 2;
	    		}

   	    		//printf("========================================================================\n");
   	    		printf("first text:%s second text:%s text dis:%1.4f\n",
   	    				first_text.text_str, second_text.text_str, text_dist);
    			//printf("========================================================================\n");
    			sim = text_dist;
                return 0;

    		}
    return -1;

}

#ifdef DEBUG
int main(int argc, char **argv){
	if (argc != 3) {
	    fprintf(stderr, "Usage: ./%s <dict_dir> <module_file>\n", argv[0]);
	    return 0;
	}

	const char* dict = argv[1];
    const char* model_file = argv[2];

    word2vec_s s;
    int ret = word2vec_init(s, model_file, dict);

    fprintf(stderr, "ret = %d\n", ret);

    double sim;
    std::string t1 = "扫黄";
    std::string t2 = "扫描2";
    ret = word2vec_calc(s, t1, t2, sim);
    fprintf(stderr, "ret=%d sim=%f\n", ret, sim);

    word2vec_destroy(s);   
}
#endif

