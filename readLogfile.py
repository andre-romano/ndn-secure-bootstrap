#!/bin/python3
import re
import fileinput
import sys
import os

class LogfileParser():

    def __init__(self):
        # last time data was saved into dat file
        self.last_time = 0

        # reset counters
        self.resetCounters()

        self.data = [[
            "Time(1)", "inInterests(2)", "satisfiedInterests(3)",
            "unsatisfiedInterests(4)", "contentStoreMisses(5)",
            "contentStoreHits(6)", "outNacks(7)", "inDatas(8)", "outDatas(9)",
            "unsolicitedDatas(10)", "timeoutInterests(11)"
        ]]

    def resetCounters(self):
        self.inInterests = 0
        self.satisfiedInterests = 0
        self.unsatisfiedInterests = 0
        self.contentStoreMisses = 0
        self.contentStoreHits = 0
        self.outNacks = 0
        self.inDatas = 0
        self.outDatas = 0
        self.unsolicitedDatas = 0
        self.timeoutInterests = 0

    def save_to_data(self):
        new_line = []
        new_line.append(self.last_time)
        new_line.append(self.inInterests)
        new_line.append(self.satisfiedInterests)
        new_line.append(self.unsatisfiedInterests)
        new_line.append(self.contentStoreMisses)
        new_line.append(self.contentStoreHits)
        new_line.append(self.outNacks)
        new_line.append(self.inDatas)
        new_line.append(self.outDatas)
        new_line.append(self.unsolicitedDatas)
        new_line.append(self.timeoutInterests)

        self.data.append(new_line)
        self.last_time += 1
        self.resetCounters()

    def parseLine(self, line):
        # only read lines with timestamp
        if (line[0] != "+"):
            return
        # ignore NFD events/msg
        if (re.search("/localhost/nfd/", line)):
            return
        # ignore appFace://, internal:// and 'all' faces
        if (re.search("(in|out)=\((258|256|2|1|-1),", line)):
            return
        split = line.split(" ")
        time = float(split[0][1:-1])
        # guarde dados no arquivo dat para graficos no gnuplot
        if (time - self.last_time >= 1):
            self.save_to_data()
        node = split[1]
        event = split[2]
        if (re.search("Forwarder:onIncomingInterest\(", event) != None):
            self.inInterests += 1
        if (re.search("Forwarder:onInterestFinalize\(", event)):
            state = split[6]
            if (re.search("^satisfied", state)):
                self.satisfiedInterests += 1
            elif (re.search("^unsatisfied", state)):
                self.unsatisfiedInterests += 1
        if (re.search("Forwarder:onContentStoreMiss\(", event)):
            self.contentStoreMisses += 1
        if (re.search("Forwarder:onContentStoreHit\(", event)):
            self.contentStoreHits += 1
        if (re.search("Forwarder:onOutgoingNack\(", event)):
            self.outNacks += 1
        if (re.search("Forwarder:onIncomingData\(", event)):
            if (re.search("^in=\(", split[5])):
                self.inDatas += 1
        if (re.search("Forwarder:onOutgoingData\(", event)):
            self.outDatas += 1
        if (re.search("Forwarder:onDataUnsolicited\(", event)):
            self.unsolicitedDatas += 1
        if (re.search("Consumer:OnTimeout\(", event)):
            self.timeoutInterests += 1

    def run(self, logfile, datafile):
        # parse logfile for data
        filename = logfile
        if not os.path.isfile(logfile):
            filename = sys.argv[1:]
        for line in fileinput.input(filename):
            self.parseLine(line)
        # save last pending data into dataset
        self.save_to_data()
        with open(datafile, 'w') as f:
            for line in self.data:
                for item in line:
                    f.write(str(item) + "\t")
                f.write("\n")
        print("--- Read Logfile ---")
        print(self.data)
        print(" ")


if __name__ == "__main__":
    os.chdir(os.path.dirname(__file__))
    p = LogfileParser()
    p.run('../../results/logfile.log', '../../results/data.dat')

# print("In Interests: ", inInterests)
# print("Sat Interests: ", satisfiedInterests)
# print("UnSat Interests: ", unsatisfiedInterests)
# print("Cont Store Miss: ", contentStoreMisses)
# print("Cont Store Hit: ", contentStoreHits)
# print("Out Nacks: ", outNacks)
# print("In Data: ", inDatas)
# print("Out Data: ", outDatas)
# print("UnSol Data: ", unsolicitedDatas)
