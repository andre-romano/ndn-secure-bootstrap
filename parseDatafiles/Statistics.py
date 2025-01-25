#!/bin/python3
from statistics import mean, median, stdev, variance

import re
import logging

# logger obj
logger = logging.getLogger(__name__)


def calculateAvg(regex: re.Pattern, nodeData: dict):
    foundOne = False
    res = 0.0
    for item, value in nodeData.items():
        if not regex.search(item):
            continue
        avg = mean(value)
        if foundOne:
            res = (res + avg)/2.0
        else:
            res = avg
    return res


def calculateAccumulation(regex: re.Pattern, nodeData: dict):
    res = 0.0
    for item, value in nodeData.items():
        if not regex.search(item):
            continue
        res += sum(value)
    return res


def calculateQtd(regex: re.Pattern, nodeData: dict):
    res = 0
    for item, value in nodeData.items():
        match = regex.search(item)
        if not match:
            continue
        res += 1
    return res


def incrementPrefix(operationsList: list, nodeData: dict, pkt_name: str, inc: int = 1):
    """ Increments a prefix by a given number `inc`

        operationsList = [(prefix, compiled_regex), ...]           
        nodeData = where to store results
    """
    for prefix, regex in operationsList:
        if not regex.search(pkt_name):
            continue
        if prefix not in nodeData:
            nodeData[prefix] = 0
        nodeData[prefix] += inc


def avgPrefix(operationsList: list, nodeData: dict, pkt_name: str, value: float):
    """ Increments a prefix by a given number `inc`

        operationsList = [(prefix, compiled_regex), ...]           
        nodeData = where to store results
    """
    for prefix, regex in operationsList:
        if not regex.search(pkt_name):
            continue
        if prefix not in nodeData:
            nodeData[prefix] = value
        else:
            nodeData[prefix] = (nodeData[prefix] + value) / 2.0
