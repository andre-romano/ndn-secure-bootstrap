#!/bin/python3

import logging

from LogfileParser import LogfileParser

# Configuração básica do logger
logging.basicConfig(level=logging.DEBUG,
                    format='[%(levelname)s]  %(name)s.%(funcName)s():\t%(message)s')

# run script
if __name__ == "__main__":
    p = LogfileParser()
    p.run('../../../results/logfile.log',
          '../../../results/dataPython.json')
