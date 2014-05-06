#ifndef _UTILITY_H
#define _UTILITY_H

#include <stddef.h>

//#include "Const.h"
typedef unsigned short gchar_t;
#define SENSIZE 20480


//半角转全角
// return : 全角字符的个数,不包含最后的0， 失败返回-1
int dbc2sbc(const char *p, char *q, int *map, int qlen);
int dbc2sbc1(const char *p, char *q, int *map, int& b, const int end);
//全角转半角
int sbc2dbc(const char *p, char *q, int qlen);
//中文空格
static const gchar_t CHINESE_SPACE_CHAR = 0xA1A1;
//常用的另一个中文空格
static const gchar_t CHINESE_SPACE_CHAR2 = 0xA0A3;
//ASCII的空格转换后的结果
static const gchar_t SPACE_CONVERSION_CHAR = 0xA0A1;
//ASCII的tab转换后的结果
static const gchar_t TAB_CONVERSION_CHAR = 0x0920;

static const gchar_t COMMA_CONVERSION_CHAR = 0xACA3;
static const gchar_t PERIOD_CONVERSION_CHAR = 0xA3A1;
static const gchar_t COLON_CONVERSION_CHAR = 0xBAA3;
static const gchar_t SEMICOLON_CONVERSION_CHAR = 0xBBA3;
static const gchar_t Q_MARK_CONVERSION_CHAR = 0xBFA3;
static const gchar_t EXCLAMATION_CONVERSION_CHAR = 0xA1A3;

#endif /*_UTILITY_H*/
