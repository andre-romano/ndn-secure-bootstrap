#!/bin/python3
from statistics import mean, median, stdev, variance

import re
import fileinput
import sys
import os
import json


class LogfileParser:
    def __init__(self):
        # saida json
        self.data = {}

        # key regex
        self.key_regex = re.compile(
            r"(.+/KEY/[^/]+)")

        # sign regex
        self.sign_regex = re.compile(
            r"(.+/SIGN/.+)")

        # SCHEMA regex
        self.schema_subs_regex = re.compile(
            r"(.+/SCHEMA/SUBSCRIBE)")
        self.schema_cont_regex = re.compile(
            r"(.+/SCHEMA/CONTENT)")

        # PAYLOAD regex
        self.payload_regex = re.compile(
            r"^(/zone[^/]+/test/prefix/node_[0-9]+/app_[0-9]+)$")

        # Dicionário para armazenar tempos de envio de Interesse por nó
        self.interest_timestamps = {}

    def calculateAvg(self, prefix: str, regex: re.Pattern, nodeData: dict):
        itemsLen = 0
        for item, value in nodeData.copy().items():
            match = regex.search(item)
            if not match:
                continue
            avg = mean(value)
            if prefix not in nodeData:
                nodeData[prefix] = avg
            else:
                nodeData[prefix] = (nodeData[prefix] + avg)/2.0
            # remove excess data from file
            del nodeData[item]

    def calculateAccumulation(self, prefix: str, regex: re.Pattern, nodeData: dict):
        for item, value in nodeData.copy().items():
            match = regex.search(item)
            if not match:
                continue
            if prefix not in nodeData:
                nodeData[prefix] = 0.0
            nodeData[prefix] += sum(value)
            # remove excess data from file
            del nodeData[item]

    def getKeyFromCertName(self, name: str):
        # verificar se é role
        key_match = self.key_regex.search(name)
        if not key_match:
            return name

        # verificar tipo de
        key_name = key_match.groups()[0]
        return key_name

    def parseRole(self, line):
        role_regex = re.compile(
            r"\+(\d+\.\d+)s (\d+) (.+):StartApplication\(\)")

        # verificar se é role
        role_match = role_regex.search(line)
        if not role_match:
            return

        # verificar tipo de
        timestamp, node, role_type = role_match.groups()

        # Atualizar hash table com o nó
        if node not in self.data:
            self.data[node] = {}
        self.data[node]['role'] = role_type

    def parseInterest(self, line):
        interest_regex = re.compile(
            r"\+(\d+\.\d+)s (\d+) CustomApp:sendInterest\(\): .* Interest packet: (.+)\?.+")

        # verificar se é interesse
        interest_match = interest_regex.search(line)
        if not interest_match:
            return

        # print("int match", line)
        # adicionar timestamp
        timestamp, node, interest_name = interest_match.groups()
        interest_name = self.getKeyFromCertName(interest_name)
        # print("int match -", timestamp, node, interest_name)
        if node not in self.interest_timestamps:
            self.interest_timestamps[node] = {}
        self.interest_timestamps[node][interest_name] = float(timestamp)

    def parseData(self, line):
        data_regex = re.compile(
            r"\+(\d+\.\d+)s (\d+) CustomApp:OnData\(\):.* Data packet: (.+) - KeyLocator")

        # verificar se é dados
        data_match = data_regex.search(line)
        if not data_match:
            return

        # print("data match", line)
        # calcular intervalo (tempo Dados - Interesse)
        timestamp, node, data_name = data_match.groups()
        data_name = self.getKeyFromCertName(data_name)
        # print("data match -", timestamp, node, data_name)
        if node in self.interest_timestamps and data_name in self.interest_timestamps[node]:
            # Calcular intervalo de tempo
            interval = float(timestamp) - \
                self.interest_timestamps[node][data_name]

            # Atualizar hash table com o nó
            if node not in self.data:
                self.data[node] = {}

            # adicionar intervalo
            if data_name not in self.data[node]:
                self.data[node][data_name] = []
            if interval > 0:
                self.data[node][data_name].append(interval)

            # Remover o Interesse processado
            del self.interest_timestamps[node][data_name]

    def calculateSummations(self, nodeData: dict):
        self.calculateAccumulation(
            prefix="/SUM/SIGN",
            regex=self.sign_regex,
            nodeData=nodeData)
        self.calculateAccumulation(
            prefix="/SUM/KEY",
            regex=self.key_regex,
            nodeData=nodeData)

    def calculateAverages(self, nodeData: dict):
        self.calculateAvg(
            prefix="/AVG/SCHEMA/SUBSCRIBE",
            regex=self.schema_subs_regex,
            nodeData=nodeData)
        self.calculateAvg(
            prefix="/AVG/SCHEMA/CONTENT",
            regex=self.schema_cont_regex,
            nodeData=nodeData)
        self.calculateAvg(
            prefix="/AVG/PAYLOAD",
            regex=self.payload_regex,
            nodeData=nodeData)

    def run(self, logfile, jsonfile):
        print(f"Reading logfile ...")
        for line in fileinput.input(logfile):
            self.parseRole(line)  # role type (trustanchor, producer, consumer)
            self.parseInterest(line)  # pacote de Interesse
            self.parseData(line)  # recebimento pacote de Dados

        print(f"Calculating sum / avg ...")
        for nodeID, nodeData in self.data.items():
            self.calculateSummations(nodeData)
            self.calculateAverages(nodeData)

        print(f"\n---  Result  ---")
        json_dump = json.dumps(self.data, indent=2, sort_keys=True)
        print(f"data = {json_dump}")

        print(f"\nSaving result ...")
        with open(jsonfile, 'w') as fp:
            fp.write(json_dump)
        print(f"done\n")


if __name__ == "__main__":
    os.chdir(os.path.dirname(__file__))
    p = LogfileParser()
    p.run('../../results/logfile.log', '../../results/dataIntervals.json')
