#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <stdarg.h>
#include <limits>

enum LogLevel {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR
};

class Logger {
 public:
  Logger() : logger_name_("LOGGER") {}
  Logger(const char* logger_name, FILE* log_sink, LogLevel level)
      : logger_name_(logger_name), log_sink_(log_sink), log_level_(level) {}

  void Log(const LogLevel level, const char* format, ...) {
    if(level >= log_level_) {
      if(log_sink_) {
        fprintf(log_sink_, "%s	", logger_name_);
        va_list args;
        va_start (args, format);
        vfprintf (log_sink_, format, args);
        va_end (args);
      }
    }
  }

  LogLevel GetLogLevel() {
    return log_level_;
  }
  void SetLogLevel(LogLevel level) {
    log_level_ = level;
  }
 private:
  const char* logger_name_;
  FILE *log_sink_ = stderr;
  LogLevel log_level_ = INFO;
};

struct Stats {
  double mean_;
  double stdev_;
  int n_ = 0;
  double sum_ = 0.0;
  double ss_ = 0.0;
  double min_ = std::numeric_limits<double>::max();
  double max_ = std::numeric_limits<double>::min();

  void Clear() {
    n_ = 0;
    sum_ = 0.0;
    ss_ = 0.0;
    min_ = std::numeric_limits<double>::max();
    max_ = std::numeric_limits<double>::min();
  }

  void Update(double datum) {
    sum_ += datum;
    ss_ += datum * datum;
    if(datum < min_) {
      min_ = datum;
    }
    if(datum > max_) {
      max_ = datum;
    }
    n_++;
  }

  void ComputeStats(double sum, double ss, int n) {
    n_ = n;
    mean_ = sum / (double)n_;
    stdev_ = sqrt(ss / (double)n_ - (mean_ * mean_));
  }

  void ComputeStats() {
    ComputeStats(sum_, ss_, n_);
  }

};

#endif
