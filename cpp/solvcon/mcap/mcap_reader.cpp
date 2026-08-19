/*
 * Copyright (c) 2026, solvcon team <contact@solvcon.net>
 * BSD 3-Clause License, see COPYING
 */

#include <solvcon/mcap/mcap_reader.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace solvcon
{

namespace mcap
{

namespace detail
{

constexpr char MAGIC[8] = {'\x89', 'M', 'C', 'A', 'P', '0', '\r', '\n'};
constexpr uint64_t MAGIC_SIZE = sizeof(MAGIC);
// One opcode byte and one uint64 content length.
constexpr uint64_t RECORD_HEADER_SIZE = 9;
// Two uint64 offsets and one uint32 CRC.
constexpr uint64_t FOOTER_CONTENT_SIZE = 20;

enum Opcode : uint8_t
{
    OP_FOOTER = 0x02,
    OP_SCHEMA = 0x03,
    OP_CHANNEL = 0x04,
    OP_CHUNK_INDEX = 0x08,
    OP_STATISTICS = 0x0b,
}; /* end enum Opcode */

/**
 * @internal
 * Cursor reading the fields of one record content.  MCAP writes its integers
 * little-endian, and so does every platform solvcon builds for, so a field is
 * a plain copy out of the buffer.
 */
class FieldReader
{

public:

    explicit FieldReader(std::string_view data)
        : m_data(data)
    {
    }

    uint16_t u16() { return read<uint16_t>(); }
    uint32_t u32() { return read<uint32_t>(); }
    uint64_t u64() { return read<uint64_t>(); }

    std::string_view str() { return bytes(u32()); }

    std::string_view bytes(uint64_t length)
    {
        require(length);
        std::string_view const out = m_data.substr(m_pos, length);
        m_pos += length;
        return out;
    }

    std::string_view rest() { return bytes(m_data.size() - m_pos); }

    void skip(uint64_t length) { bytes(length); }

    bool done() const { return m_pos >= m_data.size(); }

private:

    template <typename T>
    T read()
    {
        require(sizeof(T));
        T value = 0;
        std::memcpy(&value, m_data.data() + m_pos, sizeof(T));
        m_pos += sizeof(T);
        return value;
    }

    void require(uint64_t length) const
    {
        if (m_pos + length > m_data.size())
        {
            throw std::runtime_error("MCAP record is truncated");
        }
    }

    std::string_view m_data;
    uint64_t m_pos = 0;
}; /* end class FieldReader */

bool next_record(std::string_view buffer, size_t & pos, uint8_t & opcode, std::string_view & content)
{
    if (buffer.size() - pos < RECORD_HEADER_SIZE)
    {
        return false;
    }

    opcode = static_cast<uint8_t>(buffer[pos]);
    uint64_t length = 0;
    std::memcpy(&length, buffer.data() + pos + 1, sizeof(length));
    if (buffer.size() - pos - RECORD_HEADER_SIZE < length)
    {
        throw std::runtime_error("MCAP record runs past the end of its section");
    }

    content = buffer.substr(pos + RECORD_HEADER_SIZE, length);
    pos += RECORD_HEADER_SIZE + length;
    return true;
}

} /* end namespace detail */

Reader::Reader(std::string const & path)
    : m_path(path)
    , m_stream(path, std::ios::binary)
{
    if (!m_stream)
    {
        throw std::runtime_error("cannot open the MCAP file: " + path);
    }

    m_stream.seekg(0, std::ios::end);
    m_file_size = static_cast<uint64_t>(m_stream.tellg());
    read_summary();
}

std::string Reader::read_bytes(uint64_t offset, uint64_t length)
{
    // Both arguments come out of the file.  A sum of the two wraps back into
    // range for a large enough pair, so the check avoids the addition.
    if (offset > m_file_size || length > m_file_size - offset)
    {
        throw std::runtime_error("MCAP read runs past the end of the file: " + m_path);
    }

    std::string out(length, '\0');
    m_stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    m_stream.read(out.data(), static_cast<std::streamsize>(length));
    if (!m_stream)
    {
        throw std::runtime_error("cannot read the MCAP file: " + m_path);
    }

    return out;
}

void Reader::read_summary()
{
    using namespace detail;

    constexpr uint64_t TAIL_SIZE = RECORD_HEADER_SIZE + FOOTER_CONTENT_SIZE + MAGIC_SIZE;
    if (m_file_size < MAGIC_SIZE + TAIL_SIZE)
    {
        throw std::runtime_error("not an MCAP file, it is too short: " + m_path);
    }

    std::string const head = read_bytes(0, MAGIC_SIZE);
    std::string const tail = read_bytes(m_file_size - TAIL_SIZE, TAIL_SIZE);
    if (0 != std::memcmp(head.data(), MAGIC, MAGIC_SIZE) ||
        0 != std::memcmp(tail.data() + tail.size() - MAGIC_SIZE, MAGIC, MAGIC_SIZE))
    {
        throw std::runtime_error("not an MCAP file, the magic does not match: " + m_path);
    }

    size_t pos = 0;
    uint8_t opcode = 0;
    std::string_view content;
    next_record(tail, pos, opcode, content);
    if (OP_FOOTER != opcode || FOOTER_CONTENT_SIZE != content.size())
    {
        throw std::runtime_error("the MCAP footer record is malformed: " + m_path);
    }

    FieldReader footer(content);
    uint64_t const summary_start = footer.u64();
    if (0 == summary_start)
    {
        throw std::runtime_error("the MCAP file carries no summary section: " + m_path);
    }
    uint64_t const summary_end = m_file_size - TAIL_SIZE;
    if (summary_start >= summary_end)
    {
        throw std::runtime_error("the MCAP summary section starts at or past the footer: " + m_path);
    }

    parse_summary(read_bytes(summary_start, summary_end - summary_start));

    if (m_has_time_range || m_chunk_indices.empty())
    {
        return;
    }
    // No statistics record; the chunk indexes bound the same range.
    m_has_time_range = true;
    m_start_time = m_chunk_indices.front().message_start_time;
    for (ChunkIndexRecord const & chunk : m_chunk_indices)
    {
        m_start_time = std::min(m_start_time, chunk.message_start_time);
        m_end_time = std::max(m_end_time, chunk.message_end_time);
    }
}

void Reader::parse_summary(std::string_view records)
{
    using namespace detail;

    size_t pos = 0;
    uint8_t opcode = 0;
    std::string_view content;
    while (next_record(records, pos, opcode, content))
    {
        FieldReader field(content);
        switch (opcode)
        {
        case OP_SCHEMA:
        {
            SchemaRecord schema;
            schema.id = field.u16();
            schema.name = std::string(field.str());
            schema.encoding = std::string(field.str());
            schema.data = std::string(field.bytes(field.u32()));
            m_schemas[schema.id] = std::move(schema);
            break;
        }
        case OP_CHANNEL:
        {
            ChannelRecord channel;
            channel.id = field.u16();
            channel.schema_id = field.u16();
            channel.topic = std::string(field.str());
            channel.message_encoding = std::string(field.str());
            m_channels[channel.id] = std::move(channel);
            break;
        }
        case OP_CHUNK_INDEX:
        {
            ChunkIndexRecord chunk;
            chunk.message_start_time = field.u64();
            chunk.message_end_time = field.u64();
            chunk.chunk_start_offset = field.u64();
            chunk.chunk_length = field.u64();
            FieldReader offsets(field.str());
            while (!offsets.done())
            {
                chunk.channel_ids.push_back(offsets.u16());
                offsets.u64();
            }
            m_chunk_indices.push_back(std::move(chunk));
            break;
        }
        case OP_STATISTICS:
        {
            // The message, schema, channel, attachment, metadata, and chunk
            // counts come before the time range.
            field.skip(8 + 2 + 4 + 4 + 4 + 4);
            m_has_time_range = true;
            m_start_time = field.u64();
            m_end_time = field.u64();
            break;
        }
        default:
            break;
        }
    }
}

std::map<std::string, std::string> Reader::topics() const
{
    std::map<std::string, std::string> out;
    for (auto const & pair : m_channels)
    {
        auto const it = m_schemas.find(pair.second.schema_id);
        out[pair.second.topic] = m_schemas.end() == it ? std::string() : it->second.name;
    }

    return out;
}

} /* end namespace mcap */

} /* end namespace solvcon */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
