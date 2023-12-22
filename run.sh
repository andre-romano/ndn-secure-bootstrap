#!/bin/bash

# @echo off

#source ~/.bashrc
# source ../custom/.bashrc

# set exit status of the pipeline to the rightmost command with nonzero exit status
set -o pipefail

# important paths and files definition
RESULTS_DIR=~/ndnSIM/ns-3/results
LOGFILE=${RESULTS_DIR}/logfile.log
SIMULATION_LOG=${RESULTS_DIR}/simulation.log
MOBILITY_LOG=${RESULTS_DIR}/mobility-trace.log
PARSE_LOGFILE=${RESULTS_DIR}/readLogfile.py
CREATE_MOBILITY_TRACE=./createNS3mobilityTrace.sh

mkdir -p $RESULTS_DIR

# run a script within a specific working dir, return back 
# to current working dir after script terminated
run_from_dir(){
    CUR_PWD=$(pwd) 
    cd $(dirname "$1") 
    "$1"
    cd "$CUR_PWD"
}

# auxiliary function to facilitate waf usage,
# regardless of current working dir, or variable exports
waf(){
  APP="$1"
  export NS_LOG="$2"
  export NS_GLOBAL="$3"
  shift; shift ; shift

  CUR_PWD=$(pwd)

  cd ~/ndnSIM/ns-3 &&
  ./waf --run="$APP" $@ 2>&1 &&
  cd "$CUR_PWD"
}

if [ -z "$NODE_NUM" ]; then
    # total number of network nodes (consumers + producers + forwarders)
    NODE_NUM=20
fi
export NODE_NUM

if [ -z "$SIM_DURATION" ]; then
    # simulation durantion in seconds
    SIM_DURATION=11
fi
export SIM_DURATION

if [ -z "$MAP_SIZE" ]; then
    # map size used by BonnMotion mobility models
    MAP_SIZE=200
fi
export MAP_SIZE

if [ -z "$MOBILITY_MODEL" ]; then
    # default mobility model used (available ones are Static, RPGM, RandomWalk, RandomWaypoint)    
    MOBILITY_MODEL=Static
    # to change mobility model parameters, please check the ".param" files
fi
export MOBILITY_MODEL

if [ -z "$SIM_FILE" ]; then
    # SIM_FILE="sim_ndn_wifi -nSimDuration=${SIM_DURATION} -nNodes=${NODE_NUM}"
    SIM_FILE="sim_bootsec -nSimDuration=${SIM_DURATION} -nNodes=${NODE_NUM}"
fi
if [ -z "$GLOBAL_ARGS" ]; then
    GLOBAL_ARGS="RngRun=1"
fi
if [ -z "$SHOW_LOGS" ]; then
    # set it to 0 to stop logfile generation
    SHOW_LOGS="1"
fi
if [ -z "$LOGS" ]; then    
    # :ndn-cxx.nfd-IntMetaInfo
    # :CustomTracer
    # CUSTOM_LOGS=CustomConsumer:CustomProducer:ndn-cxx.nfd.CustomStrategy:IntMetaInfo:IntBestQuartile
    CUSTOM_LOGS=sim_bootsec:CustomConsumerBoot:CustomProducerBoot
    
# :WifiRadioEnergyModelPhyListener
# :WifiRadioEnergyModel
# :BasicEnergySource
# :WifiPhyStateHelper
# :WifiPhy
# :RegularWifiMac
    # :ndn.FibHelper
    # :ndn.StackHelper
    # :ndn.NetDeviceTransport
    # :ndn-cxx.nfd.Forwarder
    # :ndn-cxx.nfd.Strategy    
    LOG_NDN_SECURITY=ndn-cxx.ndn.security.pib.Pib:ndn-cxx.ndn.security.v2.CertificateBundleFetcher:ndn-cxx.ndn.security.v2.CertificateCache:ndn-cxx.ndn.security.v2.CertificateFetcher:ndn-cxx.ndn.security.v2.CertificateFetcher.FromNetwork:ndn-cxx.ndn.security.v2.KeyChain:ndn-cxx.ndn.security.v2.TrustAnchorGroup:ndn-cxx.ndn.security.v2.ValidationState:ndn-cxx.ndn.security.v2.Validator:ndn-cxx.ndn.security.validator_config.Rule

    # Activates some logs in NS3 (modules are separed by :) - check list of available 
    # modules by typing LOGS=nshelp    
    LOGS=ndn.Producer:ndn.Consumer:${CUSTOM_LOGS}:${LOG_NDN_SECURITY}
    
elif [ "$LOGS" == ":" ]; then    
    # if user typed LOGS=: , logs will be disabled in NS3 simulation
    LOGS=""
fi

if [ -z "$REPEAT" ]; then
    # repeat simulation N times, using distinct simulation SEEDs
    REPEAT=1
fi
REPEAT_TOTAL="$REPEAT"

RESULT_INITIAL_PATH=$($GET_PATH_MOVE_RESULTS "$RESULTS_DIR")
while [ "$REPEAT" -gt 0 ]; do
    # set simulation seed
    GLOBAL_ARGS="RngRun=$REPEAT"
    export SEED=$REPEAT 
    # build mobility trace
    echo -e -n "\nBuilding mobility trace \"$MOBILITY_MODEL\" with BonnMotion ... "
    run_from_dir "$CREATE_MOBILITY_TRACE" 2>&1 > $MOBILITY_LOG &&
    echo -e "OK\n" ||
    ( echo "ERROR - BUILD MOBILITY TRACE FAILED" ; break )
    # START SIMULATION
    rm -f "$LOGFILE" "$SIMULATION_LOG"
    sync
    # describe simulation for easier debugging in logfile
    echo -e "-- Run WAF simulation --" | tee -a $SIMULATION_LOG
    echo -e "SHOW_LOGS = " $SHOW_LOGS | tee -a  $SIMULATION_LOG
    echo -e "SIM_FILE = " $SIM_FILE | tee -a  $SIMULATION_LOG
    echo -e "LOGS = " $LOGS | tee -a  $SIMULATION_LOG
    echo -e "GLOBAL_ARGS = " $GLOBAL_ARGS | tee -a  $SIMULATION_LOG
    echo -e "REPEAT= $REPEAT / $REPEAT_TOTAL \n" | tee -a  $SIMULATION_LOG 
    
    if [ $SHOW_LOGS == "1" ]; then
        waf "$SIM_FILE" "$LOGS" "$GLOBAL_ARGS" "$@" | tee "$LOGFILE" && 
        "$PARSE_LOGFILE"         
    else
        waf "$SIM_FILE" "$LOGS" "$GLOBAL_ARGS" "$@" | "$PARSE_LOGFILE"     
    fi
    # TODO parse data generated in the logfile and traces
    REPEAT=$(( $REPEAT - 1 ))
done
exit 0

