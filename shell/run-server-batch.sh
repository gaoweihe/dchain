killall -9 tc-server
rm -rf ./*.out
rm -rf ../log/*.out

for ((i = 1; i <= $1; i++)) do 
    nohup bash run-server.sh ${i} & 
done    
