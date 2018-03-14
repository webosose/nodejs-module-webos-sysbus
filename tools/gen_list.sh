#!/bin/sh

output_file=$1
shift
cat /dev/null > $output_file

comma=
for i
do
	echo $comma\"$i\" >> $output_file
	comma=,
done
