#ifndef CUSTOM_UTILS_H
#define CUSTOM_UTILS_H

#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept> // for standard exception classes
#include <stdint.h>
#include <string>
#include <vector>

namespace utils {

  class AvgStruct {
  public:
    AvgStruct();
    void update(double current);
    double get();
    void clear();

  private:
    double m_data;
    uint64_t m_count;
  };

  bool fileExists(std::string filename);

  int generateRandomInteger(int min = std::numeric_limits<int>::min(),
                            int max = std::numeric_limits<int>::max());

  std::shared_ptr<std::vector<std::string>> splitStringByDelimiter(const std::string &str, char delimiter);

} // namespace utils

#endif /* CUSTOM_UTILS_H */