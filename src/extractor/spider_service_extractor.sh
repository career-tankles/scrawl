
URL="$1"
if [ "$URL" == "BAIDU_LIST" ]; then
    ./spider_service_extractor --EXTRACTOR_input_format=BAIDU_SEARCH_LIST --EXTRACTOR_input_file=output.json --EXTRACTOR_output_file=baidu_searchlist.json
elif [ -z "$URL" ]; then
    ./spider_service_extractor --EXTRACTOR_input_tpls_file=tpls.conf --EXTRACTOR_input_format=SPIDER_SERVICE_JSON --EXTRACTOR_input_file=input.json --EXTRACTOR_output_file=output.json
else
    ua="Mozilla/5.0 (Linux; U; Android 2.3; en-us) AppleWebKit/999+ (KHTML, like Gecko) Safari/999.9"
    host=`echo $URL | awk -F'/' '{printf("http://%s", $3);}'`
    wget -U "$ua" "$URL" -O extract_test.html 
    
    ./spider_service_extractor --EXTRACTOR_input_tpls_file=tpls.conf --EXTRACTOR_input_format=HTML --EXTRACTOR_input_file=extract_test.html --EXTRACTOR_URL=$URL
fi

