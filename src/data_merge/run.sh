#!/bin/sh


make

#data_file=part-r-00000
data_file=baidu.searchlist.json
output_file=output.json

sed -i 's/"abs":/"summary":/g'  $data_file
sed -i 's/"abs2":/"summary":/g' $data_file
sed -i 's/"anchor":/"title":/g' $data_file
./data_merge /home/s/apps/CloudSearch/nlp_dict word2vec/data/result/vectors.bin $data_file $output_file

