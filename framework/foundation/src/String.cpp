#include <bdn/String.h>

namespace bdn
{

    String fromUtf32(const std::u32string &utf32String)
    {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
        return convert.to_bytes(utf32String);
    }

    String fromUtf16(const std::u16string &utf16String)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.to_bytes(utf16String);
    }

    std::tuple<StringView, StringView> split(const StringView &view, String::value_type token)
    {
        auto pos = view.find(token);
        if (pos == StringView::npos) {
            return {view, StringView()};
        }

        return {view.substr(0, pos), view.substr(pos + 1)};
    }

    void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return std::isspace(ch) == 0; }));
    }

    void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return std::isspace(ch) == 0; }).base(), s.end());
    }

    void trim(std::string &s)
    {
        ltrim(s);
        rtrim(s);
    }

    std::string ltrim_copy(std::string s)
    {
        ltrim(s);
        return s;
    }

    std::string rtrim_copy(std::string s)
    {
        rtrim(s);
        return s;
    }

    std::string trim_copy(std::string s)
    {
        trim(s);
        return s;
    }

    template <> std::string duration_to_string(std::chrono::nanoseconds duration)
    {
        return std::to_string(duration.count()) + "ns";
    }
    template <> std::string duration_to_string(std::chrono::microseconds duration)
    {
        return std::to_string(duration.count()) + "\xC2\xB5s";
    }
    template <> std::string duration_to_string(std::chrono::milliseconds duration)
    {
        return std::to_string(duration.count()) + "ms";
    }
    template <> std::string duration_to_string(std::chrono::seconds duration)
    {
        return std::to_string(duration.count()) + "s";
    }
    template <> std::string duration_to_string(std::chrono::minutes duration)
    {
        return std::to_string(duration.count()) + "min";
    }
    template <> std::string duration_to_string(std::chrono::hours duration)
    {
        return std::to_string(duration.count()) + "h";
    }
    template <> std::string duration_to_string(std::chrono::duration<long double, std::ratio<1>> duration)
    {
        return std::to_string(duration.count()) + "s";
    }
}
