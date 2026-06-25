#pragma once

#include "types.hpp"
#include <functional>
#include <string>
#include <vector>

// Forward-declare httplib types to keep this header clean.
// The actual httplib include is in client.cpp.
namespace httplib {
class Client;
struct MultipartFormData;
using MultipartFormDataItems = std::vector<MultipartFormData>;
} // namespace httplib

namespace simplellm {

class Client {
public:
    /// Construct a Client.
    /// api_key: if empty, reads SIMPLELLM_API_KEY from env. Throws Error if still empty.
    /// base_url: if empty, reads SIMPLELLM_BASE_URL from env, defaults to https://api.simplellm.eu
    /// timeout_seconds: connection + read timeout in seconds (default 120).
    explicit Client(
        const std::string& api_key = "",
        const std::string& base_url = "",
        int timeout_seconds = 120
    );

    // -----------------------------------------------------------------------
    // Chat
    // -----------------------------------------------------------------------

    /// Non-streaming chat completion.
    ChatCompletion chat_completions_create(const ChatCompletionRequest& req);

    /// Streaming chat completions. Callback returns false to abort the stream.
    void chat_completions_stream(
        const ChatCompletionRequest& req,
        std::function<bool(const ChatCompletionChunk&)> callback
    );

    // -----------------------------------------------------------------------
    // Models
    // -----------------------------------------------------------------------

    ModelList models_list();

    // -----------------------------------------------------------------------
    // Audio
    // -----------------------------------------------------------------------

    Transcription audio_transcribe(const TranscriptionRequest& req);
    std::vector<uint8_t> audio_speech(const SpeechRequest& req);

    // -----------------------------------------------------------------------
    // Images
    // -----------------------------------------------------------------------

    ImageResponse images_generate(const ImageGenerationRequest& req);

    // -----------------------------------------------------------------------
    // Usage
    // -----------------------------------------------------------------------

    AccountUsage usage();

    // -----------------------------------------------------------------------
    // Keys
    // -----------------------------------------------------------------------

    APIKeyList keys_list();
    APIKeyInfo keys_current();
    APIKeyUsage keys_current_usage();
    APIKeyDailyUsageList keys_current_daily_usage();
    APIKeyUsage keys_usage(const std::string& key_id);
    APIKeyDailyUsageList keys_daily_usage(const std::string& key_id);

private:
    std::string api_key_;
    std::string base_url_;  // full base, no trailing slash
    std::string host_;      // scheme + host, e.g. "https://api.simplellm.eu"
    std::string path_prefix_; // e.g. "" or "/v1-compat"
    int timeout_seconds_;

    // Internal helpers
    httplib::Client make_http_client() const;
    nlohmann::json do_get(const std::string& path);
    nlohmann::json do_post_json(const std::string& path, const nlohmann::json& body);
    std::vector<uint8_t> do_post_raw(const std::string& path, const nlohmann::json& body);
    void do_post_stream(
        const std::string& path,
        const nlohmann::json& body,
        std::function<bool(const std::string& line)> line_callback
    );
    nlohmann::json do_multipart(
        const std::string& path,
        const httplib::MultipartFormDataItems& items
    );
    void check_response(int status, const std::string& body);
};

} // namespace simplellm
