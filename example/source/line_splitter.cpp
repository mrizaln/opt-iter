#include <opt_iter/opt_iter.hpp>

#include <cstdio>
#include <print>
#include <ranges>

std::string file_read(const char* path)
{
    auto file = std::fopen(path, "r");
    if (file == nullptr) {
        return {};
    }

    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return {};
    }

    auto ssize = std::ftell(file);
    if (ssize < 0 || (ssize == LONG_MAX)) {
        std::fclose(file);
        return {};
    }

    auto result = std::string(static_cast<std::size_t>(ssize), '\0');

    std::rewind(file);
    auto read = std::fread(result.data(), 1, result.size(), file);
    if (read != static_cast<std::size_t>(ssize)) {
        std::fclose(file);
        return {};
    }

    std::fclose(file);
    return result;
}

class StringSplitter
{
public:
    StringSplitter(std::string_view str, char delim = ' ')
        : m_str{ str }
        , m_delim{ delim }
    {
    }

    std::optional<std::string_view> operator()()
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
    auto splitter = StringSplitter{ string, '\n' };

    // while (auto line = splitter()) {
    //     std::println("{:>8} | {}", i++, *line);
    // }

    for (auto [i, line] : opt_iter::make_fn<std::string_view>(splitter) | std::views::enumerate) {
        std::println("{:>8} | {}", i + 1, line);
    }
}
