#pragma once

namespace chipimgproc {

constexpr struct Logger{
    template<class... Args>
    void trace(Args&&... args) const {}
    template<class... Args>
    void debug(Args&&... args) const {}
    template<class... Args>
    void info(Args&&... args) const {}
    template<class... Args>
    void warn(Args&&... args) const {}
    template<class... Args>
    void error(Args&&... args) const {}
    template<class... Args>
    void critical(Args&&... args) const {}
    void set_level(int n) const {}
} log;

}