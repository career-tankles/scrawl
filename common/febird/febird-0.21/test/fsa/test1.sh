#!/bin/sh

set -e
set -x

export LD_LIBRARY_PATH=../../lib:$LD_LIBRARY_PATH

find ../.. -name '*pp' | xargs sed 's/\s\+/\n/g' > alltoken.txt

awk 'length($0)>0{printf("%s\t%d\n", $1, length($1))}' alltoken.txt > alltoken_len.txt

LC_ALL=C sort -u alltoken.txt | sed '/^$/d' > alltoken.sorted.uniq

../../tools/automata/dbg/adfa_build.exe -i alltoken_len.txt -O alltoken_len.dfa
../../tools/automata/dbg/adfa_build.exe -i alltoken_len.txt -O alltoken_len.mmap -m
../../tools/automata/dbg/dawg_build.exe -i alltoken.txt -O alltoken.dawg.mmap -m -l6

../../samples/automata/abstract_api/dbg/match_key   -i alltoken_len.dfa     int uint long byte char automata > out.read.read.match_key
../../samples/automata/abstract_api/dbg/match_key   -i alltoken_len.mmap    int uint long byte char automata > out.read.mmap.match_key
../../samples/automata/abstract_api/dbg/match_key   -i alltoken_len.mmap -m int uint long byte char automata > out.mmap.mmap.match_key
cat alltoken_len.mmap| ../../samples/automata/abstract_api/dbg/match_key    int uint long byte char automata > out.mmap.mmap.match_key
../../samples/automata/abstract_api/dbg/match_key_l -i alltoken_len.mmap    int uint long byte char automata > out.read.mmap.match_key_l

../../tools/automata/dbg/ac_build.exe -i alltoken.txt -O alltoken.auac -d alltoken.daac -m -e 1
../../samples/automata/abstract_api/dbg/ac_scan -i alltoken.auac    int uint long byte char automata > out.read.mmap.ac_scan.auac
../../samples/automata/abstract_api/dbg/ac_scan -i alltoken.daac    int uint long byte char automata > out.read.mmap.ac_scan.daac

../../samples/automata/abstract_api/dbg/idx2word 1 10 20 30 40 50 60 70 80 90 100 < alltoken.dawg.mmap  > out.read.mmap.dawg.idx2word
../../samples/automata/abstract_api/dbg/word2idx int uint long byte char automata < alltoken.dawg.mmap  > out.read.mmap.dawg.word2idx

