time ./word2vec -train ./data/train/query.txt -output ./data/result/vectors.bin -cbow 0 -size 200 -window 5 -negative 0 -hs 1 -sample 1e-3 -threads 16 -binary 1
time ./word2vec -train ./data/train/query.txt -output ./data/result/vectors.txt -cbow 0 -size 200 -window 5 -negative 0 -hs 1 -sample 1e-3 -threads 16 -binary 0
