#include "wayvault/time_series.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "wayvault/checksum.h"

namespace wayvault {

std::vector<TimeSample> sort_and_deduplicate_samples(std::vector<TimeSample> samples) {
  std::stable_sort(samples.begin(), samples.end(), [](const TimeSample& a, const TimeSample& b) {
    return a.timestamp < b.timestamp;
  });
  std::vector<TimeSample> out;
  for (const TimeSample& sample : samples) {
    if (!out.empty() && out.back().timestamp == sample.timestamp) {
      out.back().value = sample.value;
    } else {
      out.push_back(sample);
    }
  }
  return out;
}

std::vector<TimeSample> moving_average(const std::vector<TimeSample>& samples, size_t window) {
  if (window == 0 || samples.empty()) {
    return samples;
  }
  std::vector<TimeSample> out;
  out.reserve(samples.size());
  double sum = 0.0;
  for (size_t i = 0; i < samples.size(); ++i) {
    sum += samples[i].value;
    if (i >= window) {
      sum -= samples[i - window].value;
    }
    size_t denom = std::min(window, i + 1);
    out.push_back(TimeSample{samples[i].timestamp, sum / static_cast<double>(denom)});
  }
  return out;
}

std::vector<TimeSample> resample_linear(const std::vector<TimeSample>& samples, int64_t interval) {
  std::vector<TimeSample> sorted = sort_and_deduplicate_samples(samples);
  if (sorted.empty() || interval <= 0) {
    return sorted;
  }
  std::vector<TimeSample> out;
  size_t segment = 1;
  for (int64_t t = sorted.front().timestamp; t <= sorted.back().timestamp; t += interval) {
    while (segment < sorted.size() && sorted[segment].timestamp < t) {
      ++segment;
    }
    if (segment >= sorted.size()) {
      out.push_back(TimeSample{t, sorted.back().value});
      continue;
    }
    const TimeSample& right = sorted[segment];
    const TimeSample& left = sorted[segment == 0 ? 0 : segment - 1];
    if (right.timestamp == left.timestamp) {
      out.push_back(TimeSample{t, right.value});
      continue;
    }
    double ratio = static_cast<double>(t - left.timestamp) / static_cast<double>(right.timestamp - left.timestamp);
    double value = left.value + (right.value - left.value) * ratio;
    out.push_back(TimeSample{t, value});
  }
  return out;
}

TimeSeriesStats analyze_time_series(const std::vector<TimeSample>& samples) {
  TimeSeriesStats stats;
  stats.count = samples.size();
  if (samples.empty()) {
    return stats;
  }
  stats.min_value = samples.front().value;
  stats.max_value = samples.front().value;
  double sum = 0.0;
  double sum_x = 0.0;
  double sum_y = 0.0;
  double sum_xy = 0.0;
  double sum_x2 = 0.0;
  int64_t base = samples.front().timestamp;
  for (const TimeSample& sample : samples) {
    stats.min_value = std::min(stats.min_value, sample.value);
    stats.max_value = std::max(stats.max_value, sample.value);
    sum += sample.value;
    double x = static_cast<double>(sample.timestamp - base);
    sum_x += x;
    sum_y += sample.value;
    sum_xy += x * sample.value;
    sum_x2 += x * x;
  }
  stats.mean = sum / static_cast<double>(samples.size());
  double n = static_cast<double>(samples.size());
  double denom = n * sum_x2 - sum_x * sum_x;
  if (std::abs(denom) > 1e-9) {
    stats.slope = (n * sum_xy - sum_x * sum_y) / denom;
  }
  return stats;
}

std::vector<AnomalyPoint> detect_anomalies(const std::vector<TimeSample>& samples, double z_threshold) {
  std::vector<AnomalyPoint> out;
  if (samples.size() < 2) {
    return out;
  }
  TimeSeriesStats stats = analyze_time_series(samples);
  double var = 0.0;
  for (const TimeSample& sample : samples) {
    double d = sample.value - stats.mean;
    var += d * d;
  }
  double sd = std::sqrt(var / static_cast<double>(samples.size()));
  if (sd <= 1e-9) {
    return out;
  }
  for (const TimeSample& sample : samples) {
    double z = (sample.value - stats.mean) / sd;
    if (std::abs(z) >= z_threshold) {
      out.push_back(AnomalyPoint{sample, z});
    }
  }
  return out;
}

std::string format_time_series_stats(const TimeSeriesStats& stats) {
  std::ostringstream out;
  out << "count=" << stats.count
      << " min=" << stats.min_value
      << " max=" << stats.max_value
      << " mean=" << stats.mean
      << " slope=" << stats.slope;
  return out.str();
}

uint32_t time_series_hash(const std::vector<TimeSample>& samples) {
  uint32_t state = 2166136261u;
  for (const TimeSample& sample : samples) {
    state = rotate_mix(state, static_cast<uint32_t>(sample.timestamp));
    state = rotate_mix(state, static_cast<uint32_t>(sample.timestamp >> 32));
    state = rotate_mix(state, static_cast<uint32_t>(sample.value * 1000000.0));
  }
  return state;
}

}  // namespace wayvault
