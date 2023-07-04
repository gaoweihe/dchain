../build/tc-client \
    --cf ../conf/client/client.json \
    --id $1 \
    >../log/client-$1.out 2>&1 &
