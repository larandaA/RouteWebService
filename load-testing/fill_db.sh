#!/bin/bash

END=1000
BODY=""
echo "/route" > ammo.txt
for ((i=1;i<=END;i++)); do
    BODY='{"name":"Point'
    BODY+=$i
    BODY+='"}'
    curl -s -H "Content-Type: application/json" -X POST -d "$BODY" 127.0.0.1/points > /dev/null
    echo "/points/Point$i" >> ammo.txt
done
for ((i=1;i<=END;i+=10)); do
    for ((j=1;j<=END;j+=7)); do
        BODY='{"source":"Point'
        BODY+=$i
        BODY+='", "destination":"Point'
        BODY+=$j
        BODY+='", "cost":10, "time":180}'
        curl -s -H "Content-Type: application/json" -X POST -d "$BODY" 127.0.0.1/relations > /dev/null
        echo "/relations/Point$i-Point$j" >> ammo.txt
    done
done