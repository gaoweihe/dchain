killall -9 tc-server

for ((i = 1; i <= $1; i++)) do 
    nohup bash run-server.sh ${i} & 
done    
