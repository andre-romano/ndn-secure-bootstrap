
#include "custom-utils.hpp"

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