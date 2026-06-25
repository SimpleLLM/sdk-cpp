#pragma once

#include "types.hpp"
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace simplellm {

/// Parse a single SSE line of the form "data: {...}" or "data: [DONE]".
/// Returns the parsed JSON if this is a data line with JSON content.
/// Returns std::nullopt for [DONE] lines, empty lines, or non-data lines.
inline std::optional<nlohmann::json> parse_sse_data_line(const std::string& line) {
    // Skip empty lines and non-data lines
    if (line.empty()) return std::nullopt;

    const std::string prefix = "data: ";
    if (line.size() < prefix.size()) return std::nullopt;
    if (line.substr(0, prefix.size()) != prefix) return std::nullopt;

    std::string payload = line.substr(prefix.size());

    // Trim trailing whitespace/CR
    while (!payload.empty() && (payload.back() == '\r' || payload.back() == ' ')) {
        payload.pop_back();
    }

    if (payload == "[DONE]") return std::nullopt;

    try {
        return nlohmann::json::parse(payload);
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

/// Parse a complete SSE stream body into a vector of ChatCompletionChunks.
/// Stops when it encounters "data: [DONE]".
inline std::vector<ChatCompletionChunk> parse_sse_chunks(const std::string& sse_body) {
    std::vector<ChatCompletionChunk> chunks;

    // Split into lines
    std::string line;
    bool done = false;

    for (size_t i = 0; i <= sse_body.size() && !done; ++i) {
        if (i == sse_body.size() || sse_body[i] == '\n') {
            // Strip trailing CR
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Check for [DONE]
            const std::string prefix = "data: ";
            if (line.size() >= prefix.size() && line.substr(0, prefix.size()) == prefix) {
                std::string payload = line.substr(prefix.size());
                while (!payload.empty() && (payload.back() == '\r' || payload.back() == ' ')) {
                    payload.pop_back();
                }
                if (payload == "[DONE]") {
                    done = true;
                } else {
                    auto maybe_json = parse_sse_data_line(line);
                    if (maybe_json) {
                        try {
                            chunks.push_back(maybe_json->get<ChatCompletionChunk>());
                        } catch (const nlohmann::json::exception&) {
                            // Skip malformed chunks
                        }
                    }
                }
            }

            line.clear();
        } else {
            line += sse_body[i];
        }
    }

    return chunks;
}

} // namespace simplellm
