/*
 * pre_treatment.cpp
 *
 *  Created on: 2013-11-6
 *      Author: xunwu
 */

#include <algorithm>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include <map>

#include "NLPAnalysis.h" //easou分词接口定义

using namespace std;
using namespace analysis;

#define MAX_SIZE 10240


int pre_treatment_sign(NLPAnalysis* analysis, NLPAnalysisInfo* result, FILE* input_file,
		FILE* output_file, FILE* query_file, FILE* log_file, uint32_t &line_num, uint32_t ok_num){

	SegmentResult* seg_result;
	EntityResult* entity_result;

	int term_count;
	int term_offset;
	int	term_len;

	int entity_count;
	int entity_start_offset;
	int entity_end_offset;
	int entity_len;

	uint32_t fbout_len;
	char line_buf[MAX_SIZE] = {0};

	char *query = NULL;
	char *frequency = NULL;
	int query_frequency = 0;

	char query_treatment[MAX_SIZE] = {0};
	char *tmp = NULL;

	bool valid = true;


	int i, j;

	while (fgets(line_buf, 2048, input_file)) {
		query = line_buf;
		while (*query == '\t' || *query == ' ') {
			query++;
		}

		while (query[strlen(query)-1]=='\r'||query[strlen(query)-1]=='\n') {
			query[strlen(query)-1]='\0';
		}
		if(strlen(query) == 0){
			continue;
		}
		frequency = query + strlen(query) - 1;
		while(*frequency != '\t' && *frequency != ' '){
			frequency--;
		}
		*frequency = '\0';
		frequency++;
		if(strlen(frequency) > 8 || *frequency < 48 || *frequency > 57){
			continue;
		}
		tmp = query_treatment;
		strncpy(tmp, frequency, strlen(frequency));
		tmp += strlen(frequency);
		*tmp = '\t';
		tmp++;

		if(strlen(query) > 512){
			continue;
		}

		valid = true;
		analysis->analysis(query, result, 1, true);
		seg_result = result->sr;
		entity_result = result->enr;
		term_count = seg_result->termCount[1];
		entity_count = entity_result->count;
		for(i = 0, j = 0; i < term_count; i++){
			if(seg_result->termArray[1][i].feature == 19 ||
					seg_result->termArray[1][i].feature == 24){
				valid = false;
				break;
			}
			term_offset = GetTermByteOffset(seg_result, 1, i);
			if(i == term_count-1){
				term_len = strlen(query)-term_offset;
			}
			else{
				term_len = GetTermByteOffset(seg_result, 1, i+1)-term_offset;
			}
			entity_start_offset = 0;
			entity_len = 0;
			entity_end_offset = 0;
			bool ctm = false;
			for(int k = 0; k < 8 && entity_result->entityInfo[j].hot[k] < 1; k++){
				if(entity_result->entityInfo[j].type[k] < 1000){
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
				for(int k = 0; k < 8 && entity_result->entityInfo[j].hot[k] < 1; k++){
					if(entity_result->entityInfo[j].type[k] < 1000){
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
//				GetTermString(seg_result, 1, i, term, MAX_TERM_LENGTH);
				strncpy(tmp, query+term_offset, term_len);
				tmp += term_len;
				*tmp = '\t';
				tmp++;
			}
			else if(entity_start_offset == term_offset){
				if(term_len > entity_len){
//					GetTermString(seg_result, 1, i, term, MAX_TERM_LENGTH);
					strncpy(tmp, query+term_offset, term_len);
					tmp += term_len;
					*tmp = '\t';
					tmp++;
				}
				else{
//					GetEntityString(seg_result, entity_result, j, term, MAX_TERM_LENGTH);
					strncpy(tmp, query+entity_start_offset, entity_len);
					tmp += entity_len;
					*tmp = '\t';
					tmp++;
				}
			}
			else if(entity_end_offset >= term_offset){
				continue;
			}
			else if(entity_end_offset < term_offset){
//				GetTermString(seg_result, 1, i, term[term_count].term, MAX_TERM_LENGTH);
				strncpy(tmp, query+term_offset, term_len);
				tmp += term_len;
				*tmp = '\t';
				tmp++;
			}
		}
		if(valid == true){
			line_num++;
			if(line_num%10000 == 0){
				printf("line_num:%d\n", line_num);
			}
			tmp--;
			*tmp = '\0';
			fprintf(output_file, "%s\n", query_treatment);
			fprintf(query_file, "%s	%s\n", frequency, query);
			if(line_num >= ok_num){
				return 0;
			}
		}

	}
	fprintf(log_file, "OK!\n");

	return 0;
}


int main(int argc, char* argv[]) {

	if(argc < 4){
		printf("arg error!\n");
	}

	struct dirent* ent = NULL;
	DIR     *p_dir = NULL;
	char    dir[512];
	struct stat    statbuf;

	FILE *input_file = NULL;
	FILE *output_file = NULL;
	FILE *query_file = NULL;

	NLPAnalysis* analysis;
	uint32_t	mode;
	//const char* dict = "/data/xunwu/work/qs_local/nlp/0.6.6.3/dict/";
	const char* dict = "/home/s/apps/CloudSearch/nlp_dict";

	NLPAnalysisInfo* result;

	uint32_t line_num = 0;
	uint32_t ok_num = 0;


	FILE* log_file = fopen("./log/log.txt", "a+");
	if(log_file == NULL){
		printf("error:log open failed!\n");
		goto failed;
	}


	if((p_dir=opendir(argv[1]))==NULL){
		printf("Cannot open directory:%s\n", argv[1]);
		return 0;
	}

	output_file = fopen(argv[2], "w");
	if(output_file == NULL){
		printf("error:file[%s] open failed!\n", argv[2]);
		goto failed;
	}

	query_file = fopen(argv[3], "w");
	if(query_file == NULL){
		printf("error:file[%s] open failed!\n", argv[3]);
		goto failed;
	}

	ok_num = atoi(argv[4]);
	if(ok_num <= 0){
		ok_num = 0xFFFFFFF;
	}

    try{
    	analysis = NLPAnalysis::instance();
    	mode = 1;
        fprintf(log_file, "mode:%d, path:%s\n", mode, dict);
        if(analysis->Init(1, dict)){
        	fprintf(log_file, "load nlp library ok\n");
        }
        else {
        	fprintf(log_file, "load nlp library fail\n");
        	return 0;
        }

        result = analysis->CreateNLPAnalysisResult(1);
        if(result){
            fprintf(log_file, "create nlp result ok\n");
        }
        else{
        	fprintf(log_file, "create nlp result fail\n");
        	return 0;
        }

    } catch(...){
    	fprintf(log_file, "load nlp library error %s\n", dict);
    	return -1;
    }

	while((ent=readdir(p_dir))!=NULL){
		snprintf( dir, 1024,"%s/%s", argv[1], ent->d_name);
		lstat(dir, &statbuf);
		 if(!S_ISDIR(statbuf.st_mode) ){
			 input_file = fopen(dir, "r");
			if(input_file == NULL){
				printf("error:file[%s] open failed!\n", dir);
				goto failed;
			}
			pre_treatment_sign(analysis, result, input_file, output_file, query_file, log_file, line_num, ok_num);
			if(input_file){
				fclose(input_file);
				input_file = 0;
			}
		 }
	}

	fprintf(log_file, "line_num:%u, ok_num:%u\n", line_num, ok_num);




	failed:
	if(input_file){
		fclose(input_file);
		input_file = 0;
	}

	if(p_dir){
		closedir(p_dir);
		p_dir = NULL;
	}

	if(log_file){
		fclose(log_file);
	}


	if(output_file){
		fclose(output_file);
		output_file = NULL;
	}

	if(result && analysis){
		analysis->DestroyNLPAnalysisResult(result);
		result = 0;
	}

	if(analysis){
		analysis->freeAll();
		analysis = 0;
		NLPAnalysis::releaseInstance();
	}


	return 0;
}



