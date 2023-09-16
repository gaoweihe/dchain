cmd="head -n 1 ~/node-label | cut -c 1"
eval node_label=\$\($cmd\)

mkdir -p ../log
../build/tc-server \
    --cf ../conf/server-debug/server$node_label.json \
    --id $node_label \
    >../log/server-$node_label.out 2>&1 &
