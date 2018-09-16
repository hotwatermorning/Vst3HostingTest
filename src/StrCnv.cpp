#include "StrCnv.hpp"
#include <cstdlib>
#include <sstream>
#include <cwchar>
#include <utf8.h>

namespace hwm {
    std::wstring to_wstr(std::string const &str)
    {
        std::wstring dest;
        utf8::utf8to32(str.begin(), str.end(), std::back_inserter(dest));
        return dest;
    }
    
    std::wstring to_wstr(std::u16string const &str)
    {
        std::string dest;
        utf8::utf16to8(str.begin(), str.end(), std::back_inserter(dest));
        return to_wstr(dest);
    }

    std::string to_utf8(std::wstring const &str)
    {
        std::string dest;
        utf8::utf32to8(str.begin(), str.end(), std::back_inserter(dest));
        return dest;
    }
    
    std::u16string to_utf16(std::wstring const &str)
    {
        std::string tmp;
        utf8::utf32to8(str.begin(), str.end(), std::back_inserter(tmp));
        
        std::u16string dest;
        utf8::utf8to16(tmp.begin(), tmp.end(), std::back_inserter(dest));
        
        return dest;
    }
}
