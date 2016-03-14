#pragma once

#include <iostream>
#include <chrono>
#include <ctime>

// Forward declaration
int exec(const char* cmd, char* buffer, unsigned int bufferSize);

template<typename Clock, typename Duration>
std::ostream& operator<<(std::ostream &stream, const std::chrono::time_point<Clock, Duration>& time_point)
{
  const time_t time = Clock::to_time_t(time_point);
  char buffer[26];
  ctime_r(&time, buffer);
  buffer[24] = '\0';
  return stream << buffer;
}

template <typename T>
void logVar(const T& t)
{
    std::cout << t << std::endl;
}

template <typename T, typename... Rest>
void logVar(const T& t, const Rest&... rest)
{
    std::cout << t;
    logVar(rest...);
}

template <typename T, typename... Rest>
void log(const T& t, const Rest&... rest)
{
    std::cout << std::chrono::system_clock::now() << ": ";
    std::cout << t;
    logVar(rest...);
}