#pragma once
#include <string>
#include <sstream>
#include <Nucleona/language.hpp>
namespace summit{ namespace exception{
template<class T>
struct DebugException : public T
{
    template<class... ARGS>
    DebugException(
        const std::string& file,
        int line_n,
        ARGS&&... args 
    )
    : T(FWD(args)...)
    {
        std::stringstream ss;
        ss 
            << "exception throw at " 
            << file << ":" << line_n << '\n';
        ss 
            << "what(): " << T::what() << std::endl;
        dbg_msg_ = ss.str();
    }
    virtual const char* what() const noexcept {
        return dbg_msg_.c_str();
    }
private:

    std::string dbg_msg_;
};
template<class T>
auto make_debug_exception(
    const std::string& file,
    int line,
    T&& e
){
    return DebugException<std::decay_t<T>>(file, line, FWD(e));
}
template<class T>
struct MakeDebugShell {
    using Type = DebugException<T>;
};
template<class T>
struct MakeDebugShell<const T&> {
    using Type = const DebugException<T>&;
};
}}
#define debug_throw(e) throw summit::exception::make_debug_exception(__FILE__, __LINE__, e)
#define debug_shell(T) typename summit::exception::MakeDebugShell<T>::Type