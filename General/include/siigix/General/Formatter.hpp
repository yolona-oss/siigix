#ifndef FORMATTER_HPP_WNQSB740
#define FORMATTER_HPP_WNQSB740

#include <string>
#include <iomanip>

namespace sgx
{
    class Formatter {
        public:

            template<typename T>
            static std::string toHex(T, bool include0x = false);

            static std::string toLower(const std::string& str);
            static std::string toUpper(const std::string& str);

            static std::string trim(const std::string& str);

            virtual ~Formatter();
        private:
            Formatter();
    };

    inline std::string
    Formatter::trim(const std::string& str)
    {
        size_t startPos = str.find_first_not_of(" \t");
        if ( startPos == std::string::npos  )
            return "";
        size_t endPos = str.find_last_not_of(" \t");
        return str.substr(startPos, endPos - startPos + 1);
    }

    template<typename T>
    inline std::string
    Formatter::toHex(T value, bool include0x)
    {
        std::stringstream stream;
        stream << (include0x ? "0x" : "")
            << std::setfill ('0') << std::setw(sizeof(T)*2)
            << std::hex << value;
        return stream.str();
    }
} /* sgx */ 

#endif /* end of include guard: FORMATTER_HPP_WNQSB740 */
