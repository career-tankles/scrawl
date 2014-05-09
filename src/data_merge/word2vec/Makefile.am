
top_dir = ../../..

AM_CPPFLAGS = -g -I. \
			  -I${top_dir}/common/nlp/include

LIBS = -L. \
	   -L${top_dir}/common/nlp/lib -lnlpanalysis -lsynonym -lsegwords -lutility -ldictEncode -lsentencestructure -lQueryClassify \
	   -lpthread -lm

noinst_LIBRARIES = libword2vec.a
libword2vec_a_SOURCES = word2phrase.c word2vec.c word-analogy.c distance.c compute-accuracy.c similarity_calculation.cpp pre_treatment.cpp 
libword2vec_a_LIBADD = 

noinst_PROGRAMS = word2vec
word2vec_SOURCES = word2vec.c

noinst_PROGRAMS += pretreatment
pretreatment_SOURCES = pre_treatment.cpp

noinst_PROGRAMS += word2phrase
word2phrase_SOURCES = word2phrase.c

noinst_PROGRAMS += distance
distance_SOURCES = distance.c

noinst_PROGRAMS += word_analogy
word_analogy_SOURCES = word-analogy.c


noinst_PROGRAMS += similarity
similarity_SOURCES = similarity_calculation.cpp

noinst_PROGRAMS += compute_accuracy
compute_accuracy_SOURCES = compute-accuracy.c






