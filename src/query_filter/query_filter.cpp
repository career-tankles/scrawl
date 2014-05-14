/*
 * filter.cpp
 *
 *  Created on: 2014-4-8
 *      Author: xunwu
 */
#include <stdio.h>
#include <stdlib.h>

#include "query_filter.h"

bool filter_query_type[213] = {
		0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0,
		1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0,
		0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 0, 1
};

int filter_query(NLPAnalysis *nlp_analysis, NLPAnalysisInfo *analysis_info, const char *input_path, const char *output_path, const char *bad_path){
	FILE *input_file = NULL;
	FILE *output_file = NULL;
	FILE *bad_file = NULL;

	char line_buf[MAX_QUERY_LEN] = {0};
	int count = 0;

	int i = 0;
	bool is_mobile = false;
	char *ptr = NULL;
	char *query = line_buf;
	int read_num = 0;
	int do_num = 0;
	int success_num = 0;

	input_file = fopen(input_path, "r");
	if(input_file == NULL){
		goto failed;
	}

	output_file = fopen(output_path, "w");
	if(output_file == NULL){
		goto failed;
	}

	bad_file = fopen(bad_path, "w");
	if(bad_file == NULL){
		goto failed;
	}

	while(!feof(input_file)){
		fgets(query, MAX_QUERY_LEN, input_file);
		 if(++read_num%10000 == 0){
			 printf("read_num:%d\n", read_num);
		 }
		if(strlen(query) > 64){
//			printf("kkkkkkkkkkkkkkkk\n");
			continue;
		}
//		printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
		ptr = query+strlen(query);
		while(*ptr != '\t' && ptr != query){
			if(*ptr == '\n'){
				*ptr = '\0';
			}
			--ptr;
		}
		if(ptr != query){
			*ptr = '\0';
			ptr++;
		}
		else {
			continue;
		}
//		printf("bbbbbbbbbbbbbbbbbbbbbbbbbb\n");
		count = atoi(ptr);
		if(count < 2){
			continue;
		}
//		printf("eeeeeeeeeeeeeeeeeeeeeeeeee\n");
		nlp_analysis->analysis(query, analysis_info, 1, true);
		 for(i = 0; i < analysis_info->cr->type_count; i++){
			 if(analysis_info->cr->type_info[i].typeId > 212){
				 continue;
			 }
			 printf("query:%s,	type:%d\n", query,analysis_info->cr->type_info[i].typeId);
			 if(filter_query_type[analysis_info->cr->type_info[i].typeId] /*&&
				(analysis_info->cr->type_info[i].hot == HOT_TYPE_HIGHER ||
				analysis_info->cr->type_info[i].hot == HOT_TYPE_HIGH)*/){
				 is_mobile = true;
				 break;
			 }
		 }
//		 printf("ccccccccccccccccccccccccccccc\n");
		 if(is_mobile){
			 fprintf(output_file, "%s	%d\n", query, count);
			 is_mobile = false;
			 if(++success_num%10000 == 0){

				 printf("success_num:%d\n", success_num);
			 }
		 }
		 else{
			 fprintf(bad_file, "%s	%d\n", query, count);
		 }

		 if(++do_num%10000 == 0){

			 printf("do_num:%d\n", do_num);
		 }
	}

failed:
	if(input_file){
		fclose(input_file);
		input_file = 0;
	}
	if(output_file){
		fclose(output_file);
		output_file = 0;
	}
	if(bad_file){
		fclose(bad_file);
		bad_file = 0;
	}
	return 0;
}

int main(int argc, char** argv){
	if(argc < 5){
		printf("Usage: %s <nlp_dict> <intput_file> <output_need> <output_bad>\n", argv[0]);
		return 0;
	}
    const char* nlp_dict = argv[1];
    const char* input_file = argv[2];
    const char* output_need = argv[3];
    const char* output_bad = argv[4];

	NLPAnalysis *nlp_analysis = NULL;
	NLPAnalysisInfo *analysis_info = NULL;
	uint32_t result_size = 1024 << 1;
	try{
		nlp_analysis = NLPAnalysis::instance();

		nlp_analysis->Init(3, nlp_dict);

		analysis_info = nlp_analysis->CreateNLPAnalysisResult(3, (int)result_size);

	} catch(...){
		return 0;
	}
	filter_query(nlp_analysis, analysis_info, input_file, output_need, output_bad);
}


