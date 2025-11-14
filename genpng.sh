#!/bin/sh

if [ -z $1 ];then
	echo "Error! not input text"
	return
fi

dot -Tpng -o pipeline.png $1