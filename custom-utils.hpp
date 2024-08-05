#ifndef CUSTOM_UTILS_BOOT_H
#define CUSTOM_UTILS_BOOT_H

#include <stdint.h>

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

#endif /* CUSTOM_UTILS_BOOT_H */