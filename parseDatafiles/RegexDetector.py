#!/bin/python3

import re
import logging

# logger obj
logger = logging.getLogger(__name__)


class RegexDetector:
    def __init__(self, regex):
        self.callbacks = []
        self.regex = re.compile(regex)

    def _runCallbacks(self, match: re.Match):
        # logger.info("Running callbacks ...")
        for callback in self.callbacks:
            try:
                callback(*match.groups())
            except Exception as e:
                logger.error(f"{e}")

    def addCallback(self, callback):
        if callback:
            self.callbacks.append(callback)

    def search(self, string):
        return self.regex.search(string)

    def match_and_run(self, line):
        # verificar se é role
        match = self.search(line)
        if not match:
            return

        # execute callbacks with tuple
        self._runCallbacks(match)
