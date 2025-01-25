#!/bin/python3
import logging

# logger obj
logger = logging.getLogger(__name__)


class IntervalCalculator:

    def __init__(self):
        self.interest_timestamps = {}
        self.intervals = {}

    def runCallback(self, callback):
        for node, nodeData in self.intervals.items():
            callback(node, nodeData)

    def saveInterestTimestamp(self, timestamp: float, node_name: str, pkt_name: str):
        # print("int match -", timestamp, node, interest_name)
        if node_name not in self.interest_timestamps:
            self.interest_timestamps[node_name] = {}
        self.interest_timestamps[node_name][pkt_name] = float(
            timestamp)

    def saveDataRetrievalInterval(self,  timestamp: float, node_name: str, pkt_name: str):
        if not (node_name in self.interest_timestamps and
                pkt_name in self.interest_timestamps[node_name]):
            return

        # Calcular intervalo de tempo
        interval = float(timestamp) - \
            self.interest_timestamps[node_name][pkt_name]

        # Atualizar hash table com o nó
        if node_name not in self.intervals:
            self.intervals[node_name] = {}

        # adicionar intervalo
        if pkt_name not in self.intervals[node_name]:
            self.intervals[node_name][pkt_name] = []

        # add somente se intervalo > 0
        if interval > 0:
            self.intervals[node_name][pkt_name].append(interval)

        # Remover o Interesse processado
        del self.interest_timestamps[node_name][pkt_name]
