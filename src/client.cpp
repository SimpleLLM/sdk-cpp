// CPPHTTPLIB_OPENSSL_SUPPORT is defined via CMake compile definitions
#include <httplib.h>

#include "simplellm/client.hpp"
#include "simplellm/sse_parser.hpp"

#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>

namespace simplellm {

namespace {

/// Strip trailing slashes from a URL string.
std::string strip_trailing_slashes(std::string s) {
    while (!s.empty() && s.back() == '/') {
        s.pop_back();
    }
    return s;
}

/// Get env var, return empty string if not set.
std::string getenv_str(const char* name) {
    const char* val = std::getenv(name);
    return val ? std::string(val) : std::string{};
}

/// Parse "https://host/path/prefix" into host part and path prefix.
/// host  = "https://host"
/// prefix = "/path/prefix" (may be empty)
std::pair<std::string, std::string> split_base_url(const std::string& base) {
    // Find scheme end
    auto scheme_end = base.find("://");
    if (scheme_end == std::string::npos) {
        // No scheme — treat whole thing as host with no prefix
        return {base, ""};
    }
    // Find first '/' after scheme://
    auto host_end = base.find('/', scheme_end + 3);
    if (host_end == std::string::npos) {
        return {base, ""};
    }
    std::string host = base.substr(0, host_end);
    std::string prefix = base.substr(host_end);
    // Strip trailing slash from prefix
    while (prefix.size() > 1 && prefix.back() == '/') {
        prefix.pop_back();
    }
    if (prefix == "/") prefix = "";
    return {host, prefix};
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Client::Client(const std::string& api_key, const std::string& base_url, int timeout_seconds)
    : timeout_seconds_(timeout_seconds)
{
    // Resolve API key
    api_key_ = api_key.empty() ? getenv_str("SIMPLELLM_API_KEY") : api_key;
    if (api_key_.empty()) {
        throw Error("API key is required. Pass it to the constructor or set SIMPLELLM_API_KEY.", 0, "missing_api_key");
    }

    // Resolve base URL
    std::string raw_base = base_url.empty() ? getenv_str("SIMPLELLM_BASE_URL") : base_url;
    if (raw_base.empty()) {
        raw_base = "https://api.simplellm.eu";
    }
    base_url_ = strip_trailing_slashes(raw_base);

    auto [host, prefix] = split_base_url(base_url_);
    host_ = host;
    path_prefix_ = prefix;
}

// ---------------------------------------------------------------------------
// Internal: make httplib Client
// ---------------------------------------------------------------------------

httplib::Client Client::make_http_client() const {
    httplib::Client cli(host_);
    cli.set_connection_timeout(timeout_seconds_);
    cli.set_read_timeout(timeout_seconds_);
    cli.set_default_headers({{"Authorization", "Bearer " + api_key_}});
    return cli;
}

// ---------------------------------------------------------------------------
// Internal: check_response
// ---------------------------------------------------------------------------

void Client::check_response(int status, const std::string& body) {
    if (status >= 200 && status < 300) return;

    std::string msg = "HTTP " + std::to_string(status);
    std::string code;

    try {
        auto j = nlohmann::json::parse(body);
        // Try {error: {message, code}}
        if (j.contains("error") && j["error"].is_object()) {
            auto& err = j["error"];
            if (err.contains("message")) msg = err["message"].get<std::string>();
            if (err.contains("code") && !err["code"].is_null()) {
                code = err["code"].get<std::string>();
            }
        } else if (j.contains("message")) {
            msg = j["message"].get<std::string>();
            if (j.contains("code") && !j["code"].is_null()) {
                code = j["code"].get<std::string>();
            }
        }
    } catch (...) {
        // Body is not JSON — use raw body as message if non-empty
        if (!body.empty()) {
            msg += ": " + body;
        }
    }

    throw Error(msg, status, code);
}

// ---------------------------------------------------------------------------
// Internal: do_get
// ---------------------------------------------------------------------------

nlohmann::json Client::do_get(const std::string& path) {
    auto cli = make_http_client();
    auto res = cli.Get(path_prefix_ + path);
    if (!res) {
        throw Error("Transport error: " + httplib::to_string(res.error()), 0, "transport_error");
    }
    check_response(res->status, res->body);
    return nlohmann::json::parse(res->body);
}

// ---------------------------------------------------------------------------
// Internal: do_post_json
// ---------------------------------------------------------------------------

nlohmann::json Client::do_post_json(const std::string& path, const nlohmann::json& body) {
    auto cli = make_http_client();
    std::string body_str = body.dump();
    auto res = cli.Post(path_prefix_ + path, body_str, "application/json");
    if (!res) {
        throw Error("Transport error: " + httplib::to_string(res.error()), 0, "transport_error");
    }
    check_response(res->status, res->body);
    return nlohmann::json::parse(res->body);
}

// ---------------------------------------------------------------------------
// Internal: do_post_raw
// ---------------------------------------------------------------------------

std::vector<uint8_t> Client::do_post_raw(const std::string& path, const nlohmann::json& body) {
    auto cli = make_http_client();
    std::string body_str = body.dump();
    auto res = cli.Post(path_prefix_ + path, body_str, "application/json");
    if (!res) {
        throw Error("Transport error: " + httplib::to_string(res.error()), 0, "transport_error");
    }
    check_response(res->status, res->body);
    const auto& s = res->body;
    return std::vector<uint8_t>(s.begin(), s.end());
}

// ---------------------------------------------------------------------------
// Internal: do_post_stream
// ---------------------------------------------------------------------------

void Client::do_post_stream(
    const std::string& path,
    const nlohmann::json& body,
    std::function<bool(const std::string& line)> line_callback
) {
    auto cli = make_http_client();
    std::string body_str = body.dump();

    std::string buffer;
    bool user_aborted = false;

    // Build a Request manually so we can attach a content_receiver for streaming
    httplib::Request req;
    req.method = "POST";
    req.path = path_prefix_ + path;
    req.headers = {
        {"Authorization", "Bearer " + api_key_},
        {"Content-Type", "application/json"},
        {"Accept", "text/event-stream"}
    };
    req.body = body_str;

    req.content_receiver = [&](const char* data, size_t data_length,
                                uint64_t /*offset*/, uint64_t /*total_length*/) -> bool {
        buffer.append(data, data_length);

        // Extract complete lines from buffer
        size_t pos = 0;
        while (true) {
            auto nl_pos = buffer.find('\n', pos);
            if (nl_pos == std::string::npos) break;

            std::string line = buffer.substr(pos, nl_pos - pos);
            // Strip trailing CR
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            pos = nl_pos + 1;

            if (!line_callback(line)) {
                user_aborted = true;
                return false; // abort
            }
        }
        buffer.erase(0, pos);
        return true;
    };

    auto res = cli.send(req);

    if (user_aborted) return;

    if (!res) {
        throw Error("Transport error: " + httplib::to_string(res.error()), 0, "transport_error");
    }

    if (res->status < 200 || res->status >= 300) {
        check_response(res->status, buffer.empty() ? res->body : buffer);
    }
}

// ---------------------------------------------------------------------------
// Internal: do_multipart
// ---------------------------------------------------------------------------

nlohmann::json Client::do_multipart(
    const std::string& path,
    const httplib::MultipartFormDataItems& items
) {
    auto cli = make_http_client();
    auto res = cli.Post(path_prefix_ + path, items);
    if (!res) {
        throw Error("Transport error: " + httplib::to_string(res.error()), 0, "transport_error");
    }
    check_response(res->status, res->body);
    return nlohmann::json::parse(res->body);
}

// ---------------------------------------------------------------------------
// Chat
// ---------------------------------------------------------------------------

ChatCompletion Client::chat_completions_create(const ChatCompletionRequest& req) {
    nlohmann::json j = req;
    // Force stream=false for non-streaming call
    j["stream"] = false;
    auto resp = do_post_json("/v1/chat/completions", j);
    return resp.get<ChatCompletion>();
}

void Client::chat_completions_stream(
    const ChatCompletionRequest& req,
    std::function<bool(const ChatCompletionChunk&)> callback
) {
    nlohmann::json j = req;
    j["stream"] = true;

    do_post_stream("/v1/chat/completions", j, [&](const std::string& line) -> bool {
        auto maybe_json = parse_sse_data_line(line);
        if (!maybe_json) {
            // Either empty line, [DONE], or non-data line
            return true; // continue
        }
        try {
            ChatCompletionChunk chunk = maybe_json->get<ChatCompletionChunk>();
            return callback(chunk);
        } catch (const nlohmann::json::exception&) {
            return true; // skip malformed chunk
        }
    });
}

// ---------------------------------------------------------------------------
// Models
// ---------------------------------------------------------------------------

ModelList Client::models_list() {
    auto resp = do_get("/v1/models");
    return resp.get<ModelList>();
}

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

Transcription Client::audio_transcribe(const TranscriptionRequest& req) {
    httplib::MultipartFormDataItems items;

    // File data
    items.push_back({
        "file",
        std::string(req.file_data.begin(), req.file_data.end()),
        req.file_name,
        "application/octet-stream"
    });

    if (req.model) items.push_back({"model", *req.model, "", ""});
    if (req.language) items.push_back({"language", *req.language, "", ""});
    if (req.prompt) items.push_back({"prompt", *req.prompt, "", ""});
    if (req.response_format) items.push_back({"response_format", *req.response_format, "", ""});
    if (req.temperature) items.push_back({"temperature", std::to_string(*req.temperature), "", ""});

    auto resp = do_multipart("/v1/audio/transcriptions", items);
    return resp.get<Transcription>();
}

std::vector<uint8_t> Client::audio_speech(const SpeechRequest& req) {
    nlohmann::json j{{"input", req.input}};
    if (req.model) j["model"] = *req.model;
    if (req.voice) j["voice"] = *req.voice;
    if (req.response_format) j["response_format"] = *req.response_format;
    if (req.speed) j["speed"] = *req.speed;
    return do_post_raw("/v1/audio/speech", j);
}

// ---------------------------------------------------------------------------
// Images
// ---------------------------------------------------------------------------

ImageResponse Client::images_generate(const ImageGenerationRequest& req) {
    nlohmann::json j{{"prompt", req.prompt}};
    if (req.model) j["model"] = *req.model;
    if (req.n) j["n"] = *req.n;
    if (req.size) j["size"] = *req.size;
    if (req.response_format) j["response_format"] = *req.response_format;
    auto resp = do_post_json("/v1/images/generations", j);
    return resp.get<ImageResponse>();
}

// ---------------------------------------------------------------------------
// Usage
// ---------------------------------------------------------------------------

AccountUsage Client::usage() {
    auto resp = do_get("/v1/usage");
    return resp.get<AccountUsage>();
}

// ---------------------------------------------------------------------------
// Keys
// ---------------------------------------------------------------------------

APIKeyList Client::keys_list() {
    auto resp = do_get("/v1/keys");
    return resp.get<APIKeyList>();
}

APIKeyInfo Client::keys_current() {
    auto resp = do_get("/v1/keys/current");
    return resp.get<APIKeyInfo>();
}

APIKeyUsage Client::keys_current_usage() {
    auto resp = do_get("/v1/keys/current/usage");
    return resp.get<APIKeyUsage>();
}

APIKeyDailyUsageList Client::keys_current_daily_usage() {
    auto resp = do_get("/v1/keys/current/usage/daily");
    return resp.get<APIKeyDailyUsageList>();
}

APIKeyUsage Client::keys_usage(const std::string& key_id) {
    auto resp = do_get("/v1/keys/" + key_id + "/usage");
    return resp.get<APIKeyUsage>();
}

APIKeyDailyUsageList Client::keys_daily_usage(const std::string& key_id) {
    auto resp = do_get("/v1/keys/" + key_id + "/usage/daily");
    return resp.get<APIKeyDailyUsageList>();
}

} // namespace simplellm
