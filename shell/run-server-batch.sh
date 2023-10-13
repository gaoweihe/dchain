killall -9 tc-server
rm -rf ./*.out
rm -rf ../log/*.out
rm -rf /tmp/tomchain/*

for ((i = 1; i <= $1; i++)) do 
    bash run-server.sh ${i}
done    
