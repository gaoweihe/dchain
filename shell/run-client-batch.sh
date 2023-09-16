killall -9 tc-client

mkdir -p ../log
rm -rf ../log/*
rm -f ./*.prof
for ((i = 1; i <= $1; i++)) do 
    bash run-client.sh ${i}
done 
