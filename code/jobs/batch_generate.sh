#/usr/bin/env bash
# generate jobs in batch

threads=(16) # The number of threads 
inputs=(image_rgb.txt) # The name of input files

rm -f *.jobs

for f in ${inputs[@]}
do
    for t in ${threads[@]}
    do
        ./generate_jobs.sh $f $t
    done
done
