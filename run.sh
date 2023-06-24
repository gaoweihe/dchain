bash build.sh 

killall tc-server tc-client 
./build/tc-server & 
./build/tc-client & 
sleep infinity
