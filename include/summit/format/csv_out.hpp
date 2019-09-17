#pragma once
#include <iostream>
#include <string>
#include <Nucleona/language.hpp>
namespace summit::format {

constexpr class CSVOut {
    auto impl(
        std::ostream& out, 
        const std::string& delm
    ) const {
        out << '\n';
    }
    template<class Token0, class... Token>
    auto impl(
        std::ostream& out, 
        const std::string& delm, 
        Token0&& token0,
        Token&&... tokens
    ) const {
        out << delm << token0;
        impl(out, delm, FWD(tokens)...);
    }
public:
    template<class Token0, class... Token>
    auto operator()(
        std::ostream& out, 
        const std::string& delm, 
        Token0&& token0,
        Token&&... tokens
    ) const {
        out << token0;
        impl(out, delm, FWD(tokens)...);
    }
} csv_out;

}