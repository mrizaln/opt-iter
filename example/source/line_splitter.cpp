#include <opt_iter/opt_iter.hpp>

#include <filesystem>
#include <fstream>
#include <print>
#include <ranges>
#include <sstream>

namespace fs = std::filesystem;

std::string file_read(const fs::path& path)
{
    if (not fs::exists(path) or not fs::is_regular_file(path)) {
        return "";
    }

    auto file    = std::ifstream{ path };
    auto sstream = std::stringstream{};

    sstream << file.rdbuf();
    return sstream.str();
}

class StringSplitter
{
public:
    StringSplitter(std::string_view str, char delim = ' ')
        : m_str{ str }
        , m_delim{ delim }
    {
    }

    std::optional<std::string_view> next()
    {
        if (m_pos == std::string_view::npos) {
            return std::nullopt;
        }

        auto next_pos = m_str.find(m_delim, m_pos);
        auto result   = m_str.substr(m_pos, next_pos - m_pos);
        m_pos         = next_pos == std::string_view::npos ? next_pos : next_pos + 1;

        return result;
    }

    void        reset() { m_pos = 0; }
    std::size_t pos() const { return m_pos; }

private:
    std::string_view m_str;
    std::size_t      m_pos = 0;
    char             m_delim;
};

int main()
{
    auto string   = file_read(__FILE__);
    auto splitter = opt_iter::make_owned<StringSplitter>(string, '\n');

    for (auto&& [i, line] : splitter | std::views::enumerate) {
        std::println("{:>8} | {}", i + 1, line);
    }
}
