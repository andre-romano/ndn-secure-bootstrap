// custom-utils.cpp

#include "custom-utils.hpp"

namespace utils {

  AvgStruct::AvgStruct() { this->clear(); }

  void AvgStruct::clear() {
    m_count = 0;
    m_data = 0;
  }

  void AvgStruct::update(double current) {
    m_count++;
    m_data = m_data * (m_count - 1) / m_count + current / m_count;
  }

  double AvgStruct::get() { return m_data; }

  int generateRandomInteger(int min, int max) {
    // random seed generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // distribution
    std::uniform_int_distribution<> distrib(min, max);

    // generate random number
    return distrib(gen);
  }

  std::shared_ptr<std::vector<std::string>> splitStringByDelimiter(const std::string &str, char delimiter) {
    auto items = std::make_shared<std::vector<std::string>>();

    size_t start = 0;
    while(true) {
      size_t end = str.find(delimiter, start);
      items->push_back(str.substr(start, end - start));
      if(end == std::string::npos)
        break;
      start = end + 1;
    }
    return items;
  }

} // namespace utils