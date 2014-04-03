

noinst_LIBRARIES = libhttp_parser.a
libhttp_parser_a_SOURCES = http_parser.c

noinst_PROGRAMS = http_parser_tools t
http_parser_tools_SOURCES = http_parser_tools.c
http_parser_tools_LDADD = libhttp_parser.a

t_SOURCES = t.cc
t_LDADD = libhttp_parser.a
