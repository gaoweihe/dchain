../build/tc-server \
    --cf ../conf/server/server.json \
    --id $1 \
    >../log/server-$1.out 2>&1 &
