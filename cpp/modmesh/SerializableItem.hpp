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

#include <iomanip>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace modmesh
{

class SerializableItem
{
public:
    virtual std::string to_json() const = 0;
    virtual void from_json(const std::string & json) = 0;

    // TODO: Add more serialization methods, e.g., to/from binary, to/from YAML.
}; /* end class SerializableItem */

namespace detail
{

// Helper trait to check if a type is a specialization of a given template
template <template <typename...> class Template, typename T>
struct is_specialization_of : std::false_type
{
};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template, Template<Args...>> : std::true_type
{
};

// Convenience variable template
template <template <typename...> class Template, typename T>
inline constexpr bool is_specialization_of_v = is_specialization_of<Template, T>::value;

inline bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

inline bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

template <typename T>
constexpr bool is_integer_v = std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> ||
                              std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> ||
                              std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                              std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>;

template <typename T>
constexpr bool is_float_v = std::is_same_v<T, float> || std::is_same_v<T, double>;

/// Escape special characters in a string.
std::string escape_string(std::string_view str_view);

/// Trim leading and trailing whitespaces and control characters.
std::string trim_string(const std::string & str);

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

union JsonValue
{
    JsonMap object;
    JsonArray array;
    std::string expression;
}; /* end union JsonValue */

struct JsonNode;
typedef std::unique_ptr<JsonNode> JsonNodePtr;
typedef std::unordered_map<std::string, JsonNodePtr> JsonMap;
typedef std::unordered_map<std::string, JsonNodePtr> JsonArray;

struct JsonNode
{
    JsonType type;
    JsonValue value;

    JsonNode(JsonType type, const std::string & expression)
        : type(type)
    {
        parse();
    }

private:
    void parse()
    {
        if (type == JsonType::Object)
        {
            value.object = parse_object();
        }
        else if (type == JsonType::Array)
        {
            value.array = parse_array();
        }
    }
}; /* end struct JsonNode */

/// Parse the top level of a JSON string.
JsonNodePtr parse_json(const std::string & json);

template <typename T>
std::string to_json_string(const T & value)
{
    if constexpr (std::is_base_of_v<SerializableItem, T>)
    {
        return value.to_json(); /* recursive here */
    }
    else if constexpr (std::is_convertible_v<T, std::string>)
    {
        return "\"" + escape_string(value) + "\"";
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return value ? "true" : "false";
    }
    else if constexpr (is_specialization_of<std::vector, T>::value)
    {
        std::ostringstream oss;
        oss << "[";
        const char * separator = "";
        for (const auto & item : value)
        {
            oss << separator << to_json_string(item); /* recursive here */
            separator = ",";
        }
        oss << "]";
        return oss.str();
    }
    else
    {
        return std::to_string(value);
    }
}

template <typename T>
void from_json_string(const detail::JsonNodePtr & node, T & value)
{
    if (node->type == detail::JsonType::Null)
    {
        return; /* TODO: properly handle null case */
    }

    if constexpr (detail::is_integer_v<T>)
    {
        if (node->type != detail::JsonType::Number)
        {
            throw std::runtime_error("Invalid JSON format: invalid number type.");
        }
        value = std::stoll(node->value_expression);
    }
    else if constexpr (detail::is_float_v<T>)
    {
        if (node->type != detail::JsonType::Number)
        {
            throw std::runtime_error("Invalid JSON format: invalid number type.");
        }
        value = std::stod(node->value_expression);
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        if (node->type != detail::JsonType::String)
        {
            throw std::runtime_error("Invalid JSON format: invalid number type.");
        }
        value = node->value_expression.substr(1, node->value_expression.size() - 2); /* Remove quotes */
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        if (node->type != detail::JsonType::Boolean)
        {
            throw std::runtime_error("Invalid JSON format: invalid boolean type.");
        }
        if (node->value_expression == "false")
        {
            value = false;
        }
        else
        {
            value = true;
        }
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        if (node->type != detail::JsonType::String)
        {
            throw std::runtime_error("Invalid JSON format: invalid number type.");
        }
        value = node->value_expression.substr(1, node->value_expression.size() - 2); /* Remove quotes */
    }
    else if constexpr (std::is_base_of_v<SerializableItem, T>)
    {
        value.from_json(node->value_expression); /* recursive here */
    }
    else
    {
        throw std::runtime_error("Invalid JSON format: invalid type.");
    }
}

template <typename T>
void from_json_string(const detail::JsonNodePtr & node, std::vector<T> & value)
{
    if (node->type == detail::JsonType::Null)
    {
        return; /* TODO: properly handle null case */
    }

    // TODO
}

}; // namespace detail

/// Serialize a class with member variables.
/// Use `register_member("key", class.member);` to add members when using this macro
/// Note that the order of members in the JSON string is based on the order of `register_member` calls.
#define MM_DECL_SERIALIZABLE(...)                                                       \
public:                                                                                 \
    std::string to_json() const override                                                \
    {                                                                                   \
        std::ostringstream oss;                                                         \
        oss << "{";                                                                     \
        const char * separator = "";                                                    \
        auto register_member = [&](const char * name, auto && value) {                  \
            oss << separator << "\"" << name << "\":" << detail::to_json_string(value); \
            separator = ",";                                                            \
        };                                                                              \
        __VA_ARGS__                                                                     \
        oss << "}";                                                                     \
        return oss.str();                                                               \
    }                                                                                   \
                                                                                        \
    void from_json(const std::string & json) override                                   \
    {                                                                                   \
        auto json_map = detail::parse_json(json);                                       \
        auto register_member = [&](const char * name, auto && value) {                  \
            auto it = json_map.find(name);                                              \
            if (it != json_map.end())                                                   \
            {                                                                           \
                detail::from_json_string(it->second, value);                            \
            }                                                                           \
        };                                                                              \
        __VA_ARGS__                                                                     \
    }
} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
