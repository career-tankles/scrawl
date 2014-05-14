/*
 * filter.h
 *
 *  Created on: 2014-4-8
 *      Author: xunwu
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "NLPAnalysis.h"

using namespace analysis;

#define MAX_QUERY_LEN 2048

struct query_info{
	char query_str[MAX_QUERY_LEN];
	uint32_t	count;
};

int filter_query(NLPAnalysis *nlp_analysis, NLPAnalysisInfo *analysis_info, char *input_path, char *output_path);


#endif /* FILTER_H_ */
