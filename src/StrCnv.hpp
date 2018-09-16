#pragma once

#include <string>

namespace hwm {
    std::wstring to_wstr(std::string const &str);
    std::wstring to_wstr(std::u16string const &str);
    std::string to_utf8(std::wstring const &str);
    std::u16string to_utf16(std::wstring const &str);
}
