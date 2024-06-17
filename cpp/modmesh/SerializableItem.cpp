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

#include <modmesh/SerializableItem.hpp>

namespace modmesh
{

namespace detail
{

std::string escape_string(std::string_view str_view)
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

std::string trim_string(const std::string & str)
{
    const char * whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

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
            break; /* end case JsonState::Start */
        case JsonState::ObjectKey:
            if (c == '"')
            {
                key.clear();
                bool close = false; // get the key string directly

                index += 1;
                while (index < json.size())
                {
                    if (json[index] == '"')
                    {
                        close = true;
                        break;
                    }
                    key.push_back(json[index]);
                    index += 1;
                }
                if (!close)
                {
                    throw std::runtime_error("Invalid JSON format: missing closing quote for key.");
                }

                key = trim_string(key);
                state = JsonState::Column;
            }
            else
            {
                throw std::runtime_error("Invalid JSON format: missing opening quote for key.");
            }
            break; /* end case JsonState::ObjectKey */
        case JsonState::Column:
            if (c == ':')
            {
                state = JsonState::ObjectValue;
            }
            break; /* end case JsonState::Column */
        case JsonState::ObjectValue:
            value_expression.clear();

            if (c == '{')
            {
                // go to the end of the object
                while (index < json.size())
                {
                    if (json[index] == '{')
                    {
                        depth += 1;
                    }
                    else if (json[index] == '}')
                    {
                        depth -= 1;
                    }

                    if (depth == 0)
                    {
                        break;
                    }

                    value_expression.push_back(json[index]);
                    index += 1;
                }

                if (depth != 0)
                {
                    throw std::runtime_error("Invalid JSON format: missing closing bracket.");
                }

                json_map.emplace(key, std::make_unique<JsonNode>(JsonType::Object, value_expression));
            }
            else if (c == '[')
            {
                // go to the end of the array
                while (index < json.size())
                {
                    if (json[index] == '[')
                    {
                        depth += 1;
                    }
                    else if (json[index] == ']')
                    {
                        depth -= 1;
                    }

                    if (depth == 0)
                    {
                        break;
                    }

                    value_expression.push_back(json[index]);
                    index += 1;
                }

                if (depth != 0)
                {
                    throw std::runtime_error("Invalid JSON format: missing closing bracket.");
                }

                json_map.emplace(key, std::make_unique<JsonNode>(JsonType::Array, value_expression));
            }
            else
            {
                if (!is_alpha(c) && !is_digit(c) && c != '"') // check if the value is a string, number, boolean, or null
                {
                    throw std::runtime_error("Invalid JSON format: invalid value expression.");
                }

                // we assume the value is a string, number, boolean, or null, and the expression is correct
                // if the expression is not correct, the exception will be thrown when parsing the value later
                while (index < json.size() - 1)
                {
                    value_expression.push_back(json[index]);

                    char c_next = json[index + 1];
                    if (c_next == ',' || c_next == '}')
                    {
                        break;
                    }
                    index += 1;
                }

                JsonType type = JsonType::Unknown;
                if (value_expression == "true" || value_expression == "false")
                {
                    type = JsonType::Boolean;
                }
                else if (value_expression == "null")
                {
                    type = JsonType::Null;
                }
                else if (value_expression[0] == '"' && value_expression[value_expression.size() - 1] == '"')
                {
                    type = JsonType::String;
                }
                else
                {
                    bool is_number = true;
                    for (char c : value_expression)
                    {
                        if (!is_digit(c) && c != '.' && c != 'e' && c != 'E' && c != '+' && c != '-')
                        {
                            is_number = false;
                            break;
                        }
                    }
                    if (is_number)
                    {
                        type = JsonType::Number;
                    }
                    else
                    {
                        throw std::runtime_error("Invalid JSON format: invalid value expression.");
                    }
                }

                json_map.emplace(key, std::make_unique<JsonNode>(type, value_expression));
            }

            state = JsonState::Comma;

            break; /* end case JsonState::ObjectValue */
        case JsonState::Comma:
            if (c == ',')
            {
                state = JsonState::ObjectKey;
            }
            else if (c == '}')
            {
                state = JsonState::End;
            }
            break; /* end case JsonState::Comma */
        case JsonState::End:
            if (index != json.size() - 1)
            {
                throw std::runtime_error("Invalid JSON format: extra characters after closing bracket.");
            }
            break; /* end case JsonState::End */
        }
    }

    if (state != JsonState::End)
    {
        throw std::runtime_error("Invalid JSON format: missing closing bracket.");
    }

    return json_map;
}

} /* end namespace detail */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
