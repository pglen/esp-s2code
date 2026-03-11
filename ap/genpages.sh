#!/bin/bash
# Re run if changed HTML page.
#echo "Converting '.html' pages to '.h' --> output to ../common/v002/html "
#python main/conv_http.py main/html/page_1.html  ../common/v002/html/html_page1.h   index_html
#python ../common/conv_http.py ../common/index.html  ../common/index.h index_html

CONV=main/conv_http.py
OUTD=main/h
INPD=main/html

echo Converting '.html' pages to '.h' --> output to $OUTD

function convert {
   python main/conv_http.py main/html/$1.html  main/h/$1.h $2
}

convert page_x   notfound_html
convert page_1   index_html
convert page_8   manual_html
convert page_2   settings_html
convert page_3   lora_html

#echo Done.































