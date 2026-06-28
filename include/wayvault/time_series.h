#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayvault {

struct TimeSample {
  int64_t timestamp = 0;
  double value = 0.0;
};

struct TimeSeriesStats {
  double min_value = 0.0;
  double max_value = 0.0;
  double mean = 0.0;
  double slope = 0.0;
  size_t count = 0;
};

struct AnomalyPoint {
  TimeSample sample;
  double z_score = 0.0;
};

std::vector<TimeSample> sort_and_deduplicate_samples(std::vector<TimeSample> samples);
std::vector<TimeSample> moving_average(const std::vector<TimeSample>& samples, size_t window);
std::vector<TimeSample> resample_linear(const std::vector<TimeSample>& samples, int64_t interval);
TimeSeriesStats analyze_time_series(const std::vector<TimeSample>& samples);
std::vector<AnomalyPoint> detect_anomalies(const std::vector<TimeSample>& samples, double z_threshold);
std::string format_time_series_stats(const TimeSeriesStats& stats);
uint32_t time_series_hash(const std::vector<TimeSample>& samples);

}  // namespace wayvault
