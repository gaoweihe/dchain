killall -9 tc-client

for ((i = 1; i <= $1; i++)) do 
    bash run-client.sh ${i}
done 
