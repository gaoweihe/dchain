cmd="head -n 1 ~/node-label | cut -c 1-2"
eval node_label=\$\($cmd\)

mkdir -p ../log
rm -rf ../log/*
rm -f ./*.prof
rm -rf /tmp/tomchain/* 

cpulimit --limit 5 ../build/tc-server \
    --cf ../conf/server/server.json \
    --id $node_label \
#    >../log/server-$node_label.out 2>&1 &
