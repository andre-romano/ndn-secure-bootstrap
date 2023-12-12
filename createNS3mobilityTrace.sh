#!/bin/bash
if [ -z "$DEST_DIR" ]; then
    DEST_DIR="../results/mobility-trace"
fi

if [ -z "$SEED" ]; then
    SEED=1
fi

if [ -z "$SIM_DURATION" ]; then
    SIM_DURATION=11
fi

if [ -z "$NODE_NUM" ]; then
    NODE_NUM=10
fi

if [ -z "$MAP_SIZE" ]; then
    MAP_SIZE=200
fi

if [ -z "$MOBILITY_MODEL" ]; then
    MOBILITY_MODEL=RPGM
fi

if [ -z "$SCENARIO" ]; then
    SCENARIO=${MOBILITY_MODEL}
fi

echo "Creating NS3 mobility trace ... " &&
rm -f *.ns_params *.ns_movements *.movements.gz
bm -f "$SCENARIO" -I "${MOBILITY_MODEL}.params" $MOBILITY_MODEL -J 2D -d $SIM_DURATION -n $NODE_NUM -x $MAP_SIZE -y $MAP_SIZE -R $SEED &&
bm NSFile -f "$SCENARIO"  &&
cp "${SCENARIO}.ns_movements" "${DEST_DIR}.ns_movements"&&
cp "${SCENARIO}.params" "${DEST_DIR}.params" &&
echo "Trace Build - OK" ||
echo "Trace Build - FAILED"
