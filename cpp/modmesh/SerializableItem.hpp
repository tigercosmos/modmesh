#pragma once

/*
 * Copyright (c) 2024, An-Chi Liu <phy.tiger@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the copyright holder nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sstream>
#include <string_view>

namespace modmesh
{

namespace detail
{

// clang-format off
template <typename T> struct is_array_like<std::vector<T>> : std::true_type{};

// TODO: Add more array-like types, e.g., std::array, std::valarray, std::deque.
template <typename T> constexpr bool is_array_like_v = is_array_like<T>::value;
// clang-format on

/// Escape special characters in a string.
string escape_string(std::string_view str_view)
{
    std::ostringstream oss;
    for (char c : str_view)
    {
        switch (c)
        {
        case '"':
            oss << "\\\"";
            break;
        case '\\':
            oss << "\\\\";
            break;
        case '\b':
            oss << "\\b";
            break;
        case '\f':
            oss << "\\f";
            break;
        case '\n':
            oss << "\\n";
            break;
        case '\r':
            oss << "\\r";
            break;
        case '\t':
            oss << "\\t";
            break;
        default:
            if (c < 32 || c >= 127)
            {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
            }
            else
            {
                oss << c;
            }
        }
    }
    return oss.str();
}

/// Trim leading and trailing whitespaces and control characters.
std::string trim_string(const std::string & str)
{
    const char * whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

enum class JsonState
{
    Start,
    ObjectKey,
    Column,
    Comma,
    ObjectValue,
    End,
}; /* end enum class JsonState */

enum class JsonType
{
    Object,
    Array,
    String,
    Number,
    Boolean,
    Null,
    Unknown,
}; /* end enum class JsonType */

struct JsonNode
{
    JsonType type;
    std::string value_expression;
}; /* end struct JsonNode */

typedef std::unique_ptr<JsonNode> JsonNodePtr;
typedef std::unordered_map<std::string, JsonNodePtr> JsonMap;

JsonMap parse_json(const std::string & json)
{
    JsonMap json_map;
    JsonState state = JsonState::Start;
    std::string key;
    std::string value_expression;

    int depth = 0;

    for (size_t index = 0; index < json.size(); index++)
    {
        char c = json[index];
        if (c == ' ')
        {
            continue; // ignore spaces
        }

        switch (state)
        {
        case JsonState::Start:
            if (c == '{' && index == 0)
            {
                state = JsonState::ObjectKey;
            }
            else
            {
                throw std::runtime_error("Invalid JSON format: missing opening bracket.");
            }
            break;
        case JsonState::ObjectKey:
            if (c == '"')
            {
                // get the key string directly
                bool close = false while (index < json.size())
                {
                    if (json[++index] != '"')
                    {
                        key.push_back(json[index]);
                    }
                    else
                    {
                        close = true;
                        break;
                    }
                }
                if (!close)
                {
                    throw std::runtime_error("Invalid JSON format: missing closing quote for key.");
                }

                key = trim_string(value_expression);

                state = JsonState::Column;
                value_expression.clear();
            }
            else
            {
                throw std::runtime_error("Invalid JSON format: missing opening quote for key.");
            }
            break;
        case JsonState::Column:
            if (c == ':')
            {
                state = JsonState::ObjectValue;
                value_expression.clear();
            }
            break;
        case JsonState::ObjectValue:
            if (c == '{')
            {
                state = JsonState::ObjectKey;
                depth += 1;

                // go to the end of the object
                while (index < json.size())
                {
                    if (json[index] == '}' && depth == 0)
                    {
                        break;
                    }

                    if (json[index] == '{')
                    {
                        depth += 1;
                    }
                    else if (json[index] == '}')
                    {
                        depth -= 1;
                    }
                    value_expression.push_back(json[index]);
                    index += 1;
                }

                if (depth != 0)
                {
                    throw std::runtime_error("Invalid JSON format: missing closing bracket.");
                }
            }
            else
            {
                value_expression.push_back(c);
            }
            break;
        case JsonState::Comma:
            if (c == ',')
            {
                state = JsonState::ObjectKey;
            }
            else if (c == '}')
            {
                state = JsonState::End;
            }
            break;
        case JsonState::End:
            break;
        }
    }

    if (state != JsonState::End)
    {
        throw std::runtime_error("Invalid JSON format: missing closing bracket.");
    }

    return json_map;
}

}; /* end class JsonParser */

} // namespace detail

class SerializableItem;
{
public:
    virtual std::string to_json() const = 0;
    virtual void from_json(const std::string & json) = 0;

    // TODO: Add more serialization methods, e.g., to/from binary, to/from YAML.
}; /* end class SerializableItem */

/// Serialize a class with member variables.
#define DECL_MM_SERIALIZABLE(...)                                                                          \
public:                                                                                                    \
    std::string to_json() const override                                                                   \
    {                                                                                                      \
        std::ostringstream oss;                                                                            \
        oss << "{";                                                                                        \
        const char * separator = "";                                                                       \
        /* use `register("key", class.member);` to add member when using this macro */                     \
        auto register = [&](const char * name, auto && value) {                                            \
            oss << separator << "\"" << name << "\":" << to_json_string(value);                            \
            separator = ",";                                                                               \
        };                                                                                                 \
        __VA_ARGS__                                                                                        \
        oss << "}";                                                                                        \
        return oss.str();                                                                                  \
    }                                                                                                      \
                                                                                                           \
    void from_json(const std::string & json) override                                                      \
    {                                                                                                      \
        auto json_map = parse_json(json);                                                                  \
        auto set_member = [&](const char * name, auto && value) {                                          \
            auto it = json_map.find(name);                                                                 \
            if (it != json_map.end())                                                                      \
            {                                                                                              \
                from_json_string(it->second, value);                                                       \
            }                                                                                              \
        };                                                                                                 \
        __VA_ARGS__                                                                                        \
    }                                                                                                      \
                                                                                                           \
private:                                                                                                   \
    template <typename T>                                                                                  \
    std::string to_json_string(const T & value) const                                                      \
    {                                                                                                      \
        if constexpr (std::is_base_of_v<SerializableItem, T>)                                              \
        {                                                                                                  \
            return value.to_json(); /* recursive here */                                                   \
        }                                                                                                  \
        else                                                                                               \
        {                                                                                                  \
            return std::to_string(value);                                                                  \
        }                                                                                                  \
    }                                                                                                      \
                                                                                                           \
    template <typename T, typename = typename std::enable_if<std::is_convertible_v<T, std::string>>::type> \
    std::string to_json_string(const std::string & value) const                                            \
    {                                                                                                      \
        return "\"" + detail::escape_string(std::string_view(value)) + "\"";                               \
    }                                                                                                      \
                                                                                                           \
    template <typename T, typename = typename std::enable_if<is_array_like_v<T>, void>::type>              \
    std::string to_json_string(const T & vec) const                                                        \
    {                                                                                                      \
        std::ostringstream oss;                                                                            \
        oss << "[";                                                                                        \
        const char * separator = "";                                                                       \
        for (const auto & item : vec)                                                                      \
        {                                                                                                  \
            oss << separator << to_json_string(item); /* recursive here */                                 \
            separator = ",";                                                                               \
        }                                                                                                  \
        oss << "]";                                                                                        \
        return oss.str();                                                                                  \
    }                                                                                                      \
                                                                                                           \
    template <typename T>                                                                                  \
    void from_json_string(const std::string & str, T & value) const                                        \
    {                                                                                                      \
        if constexpr (std::is_same<T, int>::value)                                                         \
        {                                                                                                  \
            value = std::stoi(str);                                                                        \
        }                                                                                                  \
        else if constexpr (std::is_same<T, double>::value)                                                 \
        {                                                                                                  \
            value = std::stod(str);                                                                        \
        }                                                                                                  \
        else if constexpr (std::is_same<T, std::string>::value)                                            \
        {                                                                                                  \
            value = str.substr(1, str.size() - 2); /* Remove quotes */                                     \
        }                                                                                                  \
        else if constexpr (std::is_base_of_v<SerializableItem, T>)                                         \
        {                                                                                                  \
            value.from_json(str); /* recursive here */                                                     \
        }                                                                                                  \
        else if constexpr (std::is_same_v<T, std::vector<std::string>>)                                    \
        {                                                                                                  \
            std::istringstream iss(str.substr(1, str.size() - 2)); /* Remove brackets */                   \
            std::string item;                                                                              \
            while (std::getline(iss, item, ','))                                                           \
            {                                                                                              \
                value.push_back(item.substr(1, item.size() - 2)); /* Remove quotes */                      \
            }                                                                                              \
        }                                                                                                  \
    }
} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
