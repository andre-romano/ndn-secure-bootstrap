#!/bin/python3
import re
import fileinput
import sys
import os
import json
from token import NAME


class LogfileParser:
    def __init__(self):
        # saida json
        self.data = {}

        # Dicionário para armazenar tempos de envio de Interesse por nó
        self.interest_timestamps = {}

    def getKeyFromCertName(self, name: str):
        key_regex = re.compile(
            r"(.+/KEY/[^/]+)")

        # verificar se é role
        key_match = key_regex.search(name)
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
        print("int match -", timestamp, node, interest_name)
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
        print("data match -", timestamp, node, data_name)
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

    def parseLine(self, line):
        # Match de role type (trustanchor, producer, consumer)
        self.parseRole(line)

        # Match envio de pacote de Interesse
        self.parseInterest(line)

        # Match recebimento de pacote de Dados
        self.parseData(line)

    def run(self, logfile, jsonfile):
        print(f"Reading logfile ...")
        for line in fileinput.input(logfile):
            self.parseLine(line)

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
