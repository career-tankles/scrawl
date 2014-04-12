#!/bin/sh

cd ../..
. env.sh
cd - >/dev/null

src_data=../../data/html/data-00-005

rm -f input.json
if [ "$#" == "1" ]; then
    fgrep "$1" $src_data > input.json
    ./xpath_tools --EXTRACTOR_input_tpls_file=tpls.conf --EXTRACTOR_input_format=JSON --EXTRACTOR_input_file=input.json --EXTRACTOR_output_file=output.json
else
    ln -s $src_data input.json
    ./xpath_tools --EXTRACTOR_input_tpls_file=tpls.conf --EXTRACTOR_input_format=JSON --EXTRACTOR_input_file=input.json --EXTRACTOR_output_file=output.json > log.log 2>&1
fi

