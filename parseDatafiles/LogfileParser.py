#!/bin/python3

import re
import fileinput
import json
import logging

from Statistics import calculateAccumulation, calculateAvg
from Statistics import incrementPrefix, avgPrefix

from RegexDetector import RegexDetector
from IntervalCalculator import IntervalCalculator

# logger obj
logger = logging.getLogger(__name__)


class LogfileParser:
    # interval calculator
    interval_calculator = IntervalCalculator()

    # role, interest, data REGEXES
    role_regex = RegexDetector(
        r"\+(\d+\.\d+)s (\d+) (.+):StartApplication\(\)")
    interest_regex = RegexDetector(
        r"\+(\d+\.\d+)s (\d+) CustomApp:sendInterest\(\): .* Interest packet: (.+)\?.+ - Size: (\d+)")
    data_regex = RegexDetector(
        r"\+(\d+\.\d+)s (\d+) CustomApp:OnData\(\):.* Data packet: (.+) - KeyLocator: (.+) - Size: (\d+)")

    # KEY / SIGN regex
    key_regex = RegexDetector(r"(.+/KEY/[^/]+)")
    sign_regex = RegexDetector(r"(.+/SIGN/.+)")

    # SCHEMA regex
    schema_subs_regex = RegexDetector(r"(.+/SCHEMA/SUBSCRIBE)")
    schema_cont_regex = RegexDetector(r"(.+/SCHEMA/CONTENT)")

    # PAYLOAD regex
    payload_regex = RegexDetector(
        r"^(/zone[^/]+/test/prefix/node_[0-9]+/app_[0-9]+)$")

    def __init__(self):
        # saida json
        self.data = {}

        # add callbacks
        self.role_regex.addCallback(self.parseRole)
        self.interest_regex.addCallback(self.parseInterest)
        self.data_regex.addCallback(self.parseData)

    def getKeyFromCertName(self, name: str):
        # verificar se é role
        key_match = self.key_regex.search(name)
        if not key_match:
            return name

        # verificar tipo de
        key_name = key_match.groups()[0]
        return key_name

    def countAndStorePkts(self, node: str,  pkt_name: str):
        # execute operations
        incrementPrefix(
            operationsList=[
                ("/QTD/SUM/SIGN", self.sign_regex),
                ("/QTD/SUM/KEY", self.key_regex),
                ("/QTD/SUM/SCHEMA/SUBSCRIBE", self.schema_subs_regex),
                ("/QTD/SUM/SCHEMA/CONTENT", self.schema_cont_regex),
                ("/QTD/SUM/PAYLOAD", self.payload_regex),
            ],
            inc=1,
            pkt_name=pkt_name,
            nodeData=self.data[node])

    def storePktsSizeSum(self, node: str,  pkt_name: str, size: int):
        # execute operations
        incrementPrefix(
            operationsList=[
                ("/SIZE/SUM/SIGN", self.sign_regex),
                ("/SIZE/SUM/KEY", self.key_regex),
                ("/SIZE/SUM/SCHEMA/SUBSCRIBE", self.schema_subs_regex),
                ("/SIZE/SUM/SCHEMA/CONTENT", self.schema_cont_regex),
                ("/SIZE/SUM/PAYLOAD", self.payload_regex),
            ],
            inc=int(size),
            pkt_name=pkt_name,
            nodeData=self.data[node])

    def storePktsSizeAvg(self, node: str,  pkt_name: str, size: int):
        # execute operations
        avgPrefix(
            operationsList=[
                ("/SIZE/AVG/SIGN", self.sign_regex),
                ("/SIZE/AVG/KEY", self.key_regex),
                ("/SIZE/AVG/SCHEMA/SUBSCRIBE", self.schema_subs_regex),
                ("/SIZE/AVG/SCHEMA/CONTENT", self.schema_cont_regex),
                ("/SIZE/AVG/PAYLOAD", self.payload_regex),
            ],
            value=int(size),
            pkt_name=pkt_name,
            nodeData=self.data[node])

    def parseRole(self, timestamp, node, role_type):
        # Atualizar hash table com o nó
        if node not in self.data:
            self.data[node] = {}
        self.data[node]['role'] = role_type

    def parseInterest(self, timestamp, node, interest_name, size):
        interest_name = self.getKeyFromCertName(interest_name)
        # logger.info(
        #     f"+{timestamp}s - Node {node} - Name {interest_name} - Size {size}")

        # save interest timestamp
        self.interval_calculator.saveInterestTimestamp(
            timestamp, node, interest_name)

    def parseData(self, timestamp, node, data_name, keylocator, size):
        data_name = self.getKeyFromCertName(data_name)
        # logger.info(
        #     f"{timestamp}s - Node {node} - Name {data_name} - KeyLocator {keylocator} - Size {size}")

        # save data retrieval time
        self.interval_calculator.saveDataRetrievalInterval(
            timestamp, node, data_name)

        # calculate number of pkts in a node
        self.countAndStorePkts(node, data_name)
        # calculate pkts size - SUM / AVG
        self.storePktsSizeSum(node, data_name, size)
        self.storePktsSizeAvg(node, data_name, size)

    def parseIntervalResults(self, node, nodeData):
        # calculate summations
        operationsList = [
            ("/TIME/SUM/SIGN", calculateAccumulation(self.sign_regex, nodeData)),
            ("/TIME/SUM/KEY", calculateAccumulation(self.key_regex, nodeData)),
            ("/TIME/AVG/SCHEMA/SUBSCRIBE",
             calculateAvg(self.schema_subs_regex, nodeData)),
            ("/TIME/AVG/SCHEMA/CONTENT",
             calculateAvg(self.schema_cont_regex, nodeData)),
            ("/TIME/AVG/PAYLOAD", calculateAvg(self.payload_regex, nodeData)),
        ]

        # store results
        for prefix, calc in operationsList:
            if calc > 0:
                self.data[node][prefix] = calc

    def run(self, logfile, jsonfile):
        logger.info(f"Reading logfile ...")
        for line in fileinput.input(logfile):
            self.role_regex.match_and_run(line)
            self.interest_regex.match_and_run(line)
            self.data_regex.match_and_run(line)

        logger.info(f"Calculating results ...")
        self.interval_calculator.runCallback(self.parseIntervalResults)

        logger.info(f"---  Result  ---")
        json_dump = json.dumps(self.data, indent=2, sort_keys=True)
        logger.info(f"data = {json_dump}")

        logger.info(f"Saving result ...")
        with open(jsonfile, 'w') as fp:
            fp.write(json_dump)
        logger.info(f"done\n")
