/*
 * SenType.h
 *
 *  Created on: 2013-2-7
 *      Author: shi
 */

#ifndef SENTYPE_H_
#define SENTYPE_H_

#include "../include/EasouSeg.h"//分词接口定义
#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdio.h>

#define MAX_TERM_BYTE_LEN 100
#define MAX_TERM_COUNT 60
#define HEAD 1
#define TAIL 2
#define SEP_NUM 65536

using namespace std;

//sentence type result, including the type(class_id), corresponding rule string(rule_str), and whether
//this result is active(OK).
struct Rule_Info {
	int class_id;
	char hot;
	vector<int> entities;
	string rule_str;
	bool OK;
};

//step1, initial step
void *load_resource(const char* fname);

//step2, type tagging, can be used in multi-thread mode, iteratively
bool process_sen(SegmentResult* sr, EntityResult* er, void* p_resource,
		map<int, Rule_Info>& result, bool clean_bad);

//step3, final step
bool unload_resource(void* p_resource);

#endif /* SENTYPE_H_ */
