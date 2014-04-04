

noinst_LIBRARIES = libHttpParser.a
libHttpParser_a_SOURCES = HttpParser.cc http_parser.c 

noinst_PROGRAMS = http_parser_tools HttpParser_t
http_parser_tools_SOURCES = http_parser_tools.c 
http_parser_tools_LDADD = libhttp_parser.a

HttpParser_t_SOURCES = HttpParser_t.cc
HttpParser_t_LDADD = libHttpParser.a

