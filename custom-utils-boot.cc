
#include "custom-utils-boot.hpp"

AvgStructBoot::AvgStructBoot() {
    this->clear();
}

void AvgStructBoot::clear() {
    m_count = 0;
    m_data  = 0;
}

void AvgStructBoot::update(double current) {
    m_count++;
    m_data = m_data * (m_count - 1) / m_count + current / m_count;
}

double AvgStructBoot::get() {
    return m_data;
}