#!/bin/sh


remove(){
	for f in "$1"/*
	do
		if [[ -d "$f" ]]; 
		then 
			remove "$f" $2 
		else
			if [[ $f == $2 ]];
			then
				echo Deleting file: $f
				if [ $min == 1 ]
				then
					rm "$f"
				fi
			fi
		fi
	done
}


if [ $1 == "--test" ]
then
	min=2
	dir=$2
else
	min=1
	dir=$1
fi

if [ $# -le $min ] 
then  
	echo ERROR: Incorrect number of arguements: dirclean [--test] ROOT_DIRECTORY PATTERN
	exit 1
fi

if [[ ! -d $dir ]]; 
then 
	echo "Invalid directory: $dir "
	exit 1
fi

for a in ${*:$min+1}
do
	remove $dir $a
done



#for f in $dir/*	




