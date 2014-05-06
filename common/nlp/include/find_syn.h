/*
 * find_syn.h
 *
 *  Created on: 2013-11-18
 *      Author: sdctw
 */

#ifndef FIND_SYN_H_
#define FIND_SYN_H_

#include <stdint.h>
#include <string>
#include "../include/EasouSeg.h"
#define FIND_SYN_MAX_TERM_LEN 64
#define FIND_SYN_MAX_SYN_NUM 8
typedef unsigned int UINT32;

struct syn_term {
	UINT32 score;
	char term_str[FIND_SYN_MAX_TERM_LEN];
	const char *term;
	bool operator <(const syn_term &st) const {
		return score > st.score;
	}
};

struct syn_term_list {
	int count;
	syn_term synonym[FIND_SYN_MAX_SYN_NUM];
};
struct syn_result {
	int size;
	syn_term_list *p_syn_list;
};

//dic function
void* load_syn_dic(const char* dic_name, const char* conf_name = NULL);
void  reload_syn_dic(void* p_dic, const char* dic_name, const char* conf_name = NULL);
void  unload_dic(void* p_dic);

//result structure function
syn_result* create_syn_result(int syn_size = 16);
void reset_syn_result(syn_result* p_result);
void destroy_syn_result(syn_result* p_result);

//find synonym function
bool find_syn(void* p_dic, SegmentResult* sr, EntityResult* er, syn_result* p_result);

#endif /* FIND_SYN_H_ */

