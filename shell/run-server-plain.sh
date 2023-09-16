cmd="head -n 1 ~/node-label | cut -c 1"
eval node_label=\$\($cmd\)

../build/tc-server \
    --cf ../conf/server/server.json \
    --id $node_label \
    >../log/server-$1.out 2>&1 &
