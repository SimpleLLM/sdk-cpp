#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace simplellm {

// ---------------------------------------------------------------------------
// Error
// ---------------------------------------------------------------------------

class Error : public std::runtime_error {
public:
    int status;
    std::string code;

    Error(const std::string& msg, int status_, std::string code_ = "")
        : std::runtime_error(msg), status(status_), code(std::move(code_)) {}
};

// ---------------------------------------------------------------------------
// Core chat types
// ---------------------------------------------------------------------------

struct ToolCallFunction {
    std::string name;
    std::string arguments;
};

struct ToolCall {
    std::string id;
    std::string type;
    ToolCallFunction function;
};

struct ChatMessage {
    std::string role;
    std::optional<std::string> content;
    std::optional<std::string> name;
    std::optional<std::vector<ToolCall>> tool_calls;
    std::optional<std::string> tool_call_id;
};

struct ToolFunction {
    std::string name;
    std::optional<std::string> description;
    std::optional<nlohmann::json> parameters;
};

struct Tool {
    std::string type;
    ToolFunction function;
};

struct ChatCompletionRequest {
    std::string model;
    std::vector<ChatMessage> messages;
    std::optional<double> temperature;
    std::optional<double> top_p;
    std::optional<int> max_tokens;
    std::optional<bool> stream;
    std::optional<std::vector<std::string>> stop;
    std::optional<std::vector<Tool>> tools;
    std::optional<nlohmann::json> tool_choice;
};

struct Usage {
    int prompt_tokens;
    int completion_tokens;
    int total_tokens;
};

struct ChatCompletionChoice {
    int index;
    ChatMessage message;
    std::optional<std::string> finish_reason;
};

struct ChatCompletion {
    std::string id;
    std::string object;
    int64_t created;
    std::string model;
    std::vector<ChatCompletionChoice> choices;
    std::optional<Usage> usage;
};

struct ChatCompletionDelta {
    std::optional<std::string> role;
    std::optional<std::string> content;
    std::optional<std::vector<ToolCall>> tool_calls;
};

struct ChatCompletionChunkChoice {
    int index;
    ChatCompletionDelta delta;
    std::optional<std::string> finish_reason;
};

struct ChatCompletionChunk {
    std::string id;
    std::string object;
    int64_t created;
    std::string model;
    std::vector<ChatCompletionChunkChoice> choices;
};

// ---------------------------------------------------------------------------
// Models
// ---------------------------------------------------------------------------

struct Model {
    std::string id;
    std::string object;
    int64_t created;
    std::string owned_by;
};

struct ModelList {
    std::string object;
    std::vector<Model> data;
};

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

struct TranscriptionRequest {
    std::vector<uint8_t> file_data;
    std::string file_name;
    std::optional<std::string> model;
    std::optional<std::string> language;
    std::optional<std::string> prompt;
    std::optional<std::string> response_format;
    std::optional<double> temperature;
};

struct Transcription {
    std::string text;
};

struct SpeechRequest {
    std::string input;
    std::optional<std::string> model;
    std::optional<std::string> voice;
    std::optional<std::string> response_format;
    std::optional<double> speed;
};

// ---------------------------------------------------------------------------
// Images
// ---------------------------------------------------------------------------

struct ImageGenerationRequest {
    std::string prompt;
    std::optional<std::string> model;
    std::optional<int> n;
    std::optional<std::string> size;
    std::optional<std::string> response_format;
};

struct ImageData {
    std::optional<std::string> url;
    std::optional<std::string> b64_json;
    std::optional<std::string> revised_prompt;
};

struct ImageResponse {
    int64_t created;
    std::vector<ImageData> data;
};

// ---------------------------------------------------------------------------
// Keys & Usage
// ---------------------------------------------------------------------------

struct APIKeyInfo {
    std::string id;
    std::string name;
    std::string prefix;
    std::string user_id;
    std::string created_at;
    std::optional<std::string> last_used_at;
    bool balance_enabled;
    double balance_sc;
    double balance_limit_sc;
    std::optional<int> rate_limit_rpm;
    std::optional<int> rate_limit_tpm;
};

struct APIKeyUsage {
    int total_requests;
    int64_t total_tokens_in;
    int64_t total_tokens_out;
    double total_cost_sc;
};

struct APIKeyDailyUsage {
    std::string date;
    std::string model;
    int requests;
    int64_t tokens_in;
    int64_t tokens_out;
    double cost_sc;
};

struct APIKeyList {
    std::vector<APIKeyInfo> api_keys;
};

struct APIKeyDailyUsageList {
    std::vector<APIKeyDailyUsage> usage;
};

struct AccountUsage {
    std::optional<double> balance;
    std::optional<double> total_spent;
    std::optional<int> total_requests;
};

// ---------------------------------------------------------------------------
// JSON serialization: ToolCallFunction
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ToolCallFunction& t) {
    j = nlohmann::json{{"name", t.name}, {"arguments", t.arguments}};
}

inline void from_json(const nlohmann::json& j, ToolCallFunction& t) {
    j.at("name").get_to(t.name);
    j.at("arguments").get_to(t.arguments);
}

// ---------------------------------------------------------------------------
// JSON serialization: ToolCall
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ToolCall& t) {
    j = nlohmann::json{{"id", t.id}, {"type", t.type}, {"function", t.function}};
}

inline void from_json(const nlohmann::json& j, ToolCall& t) {
    j.at("id").get_to(t.id);
    j.at("type").get_to(t.type);
    j.at("function").get_to(t.function);
}

// ---------------------------------------------------------------------------
// JSON serialization: ChatMessage
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ChatMessage& m) {
    j = nlohmann::json{{"role", m.role}};
    if (m.content) {
        j["content"] = *m.content;
    } else {
        j["content"] = nullptr;
    }
    if (m.name) j["name"] = *m.name;
    if (m.tool_calls) j["tool_calls"] = *m.tool_calls;
    if (m.tool_call_id) j["tool_call_id"] = *m.tool_call_id;
}

inline void from_json(const nlohmann::json& j, ChatMessage& m) {
    j.at("role").get_to(m.role);
    if (j.contains("content") && !j["content"].is_null()) {
        m.content = j["content"].get<std::string>();
    }
    if (j.contains("name") && !j["name"].is_null()) {
        m.name = j["name"].get<std::string>();
    }
    if (j.contains("tool_calls") && !j["tool_calls"].is_null()) {
        m.tool_calls = j["tool_calls"].get<std::vector<ToolCall>>();
    }
    if (j.contains("tool_call_id") && !j["tool_call_id"].is_null()) {
        m.tool_call_id = j["tool_call_id"].get<std::string>();
    }
}

// ---------------------------------------------------------------------------
// JSON serialization: ToolFunction
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ToolFunction& t) {
    j = nlohmann::json{{"name", t.name}};
    if (t.description) j["description"] = *t.description;
    if (t.parameters) j["parameters"] = *t.parameters;
}

inline void from_json(const nlohmann::json& j, ToolFunction& t) {
    j.at("name").get_to(t.name);
    if (j.contains("description") && !j["description"].is_null()) {
        t.description = j["description"].get<std::string>();
    }
    if (j.contains("parameters") && !j["parameters"].is_null()) {
        t.parameters = j["parameters"];
    }
}

// ---------------------------------------------------------------------------
// JSON serialization: Tool
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const Tool& t) {
    j = nlohmann::json{{"type", t.type}, {"function", t.function}};
}

inline void from_json(const nlohmann::json& j, Tool& t) {
    j.at("type").get_to(t.type);
    j.at("function").get_to(t.function);
}

// ---------------------------------------------------------------------------
// JSON serialization: ChatCompletionRequest
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ChatCompletionRequest& r) {
    j = nlohmann::json{{"model", r.model}, {"messages", r.messages}};
    if (r.temperature) j["temperature"] = *r.temperature;
    if (r.top_p) j["top_p"] = *r.top_p;
    if (r.max_tokens) j["max_tokens"] = *r.max_tokens;
    if (r.stream) j["stream"] = *r.stream;
    if (r.stop) j["stop"] = *r.stop;
    if (r.tools) j["tools"] = *r.tools;
    if (r.tool_choice) j["tool_choice"] = *r.tool_choice;
}

// ---------------------------------------------------------------------------
// JSON serialization: Usage
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const Usage& u) {
    j = nlohmann::json{
        {"prompt_tokens", u.prompt_tokens},
        {"completion_tokens", u.completion_tokens},
        {"total_tokens", u.total_tokens}
    };
}

inline void from_json(const nlohmann::json& j, Usage& u) {
    j.at("prompt_tokens").get_to(u.prompt_tokens);
    j.at("completion_tokens").get_to(u.completion_tokens);
    j.at("total_tokens").get_to(u.total_tokens);
}

// ---------------------------------------------------------------------------
// JSON serialization: ChatCompletionChoice
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ChatCompletionChoice& c) {
    j = nlohmann::json{{"index", c.index}, {"message", c.message}};
    if (c.finish_reason) j["finish_reason"] = *c.finish_reason;
    else j["finish_reason"] = nullptr;
}

inline void from_json(const nlohmann::json& j, ChatCompletionChoice& c) {
    j.at("index").get_to(c.index);
    j.at("message").get_to(c.message);
    if (j.contains("finish_reason") && !j["finish_reason"].is_null()) {
        c.finish_reason = j["finish_reason"].get<std::string>();
    }
}

// ---------------------------------------------------------------------------
// JSON serialization: ChatCompletion
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ChatCompletion& c) {
    j = nlohmann::json{
        {"id", c.id}, {"object", c.object}, {"created", c.created},
        {"model", c.model}, {"choices", c.choices}
    };
    if (c.usage) j["usage"] = *c.usage;
}

inline void from_json(const nlohmann::json& j, ChatCompletion& c) {
    j.at("id").get_to(c.id);
    j.at("object").get_to(c.object);
    j.at("created").get_to(c.created);
    j.at("model").get_to(c.model);
    j.at("choices").get_to(c.choices);
    if (j.contains("usage") && !j["usage"].is_null()) {
        c.usage = j["usage"].get<Usage>();
    }
}

// ---------------------------------------------------------------------------
// JSON serialization: ChatCompletionDelta
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ChatCompletionDelta& d) {
    j = nlohmann::json::object();
    if (d.role) j["role"] = *d.role;
    if (d.content) j["content"] = *d.content;
    if (d.tool_calls) j["tool_calls"] = *d.tool_calls;
}

inline void from_json(const nlohmann::json& j, ChatCompletionDelta& d) {
    if (j.contains("role") && !j["role"].is_null()) {
        d.role = j["role"].get<std::string>();
    }
    if (j.contains("content") && !j["content"].is_null()) {
        d.content = j["content"].get<std::string>();
    }
    if (j.contains("tool_calls") && !j["tool_calls"].is_null()) {
        d.tool_calls = j["tool_calls"].get<std::vector<ToolCall>>();
    }
}

// ---------------------------------------------------------------------------
// JSON serialization: ChatCompletionChunkChoice
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ChatCompletionChunkChoice& c) {
    j = nlohmann::json{{"index", c.index}, {"delta", c.delta}};
    if (c.finish_reason) j["finish_reason"] = *c.finish_reason;
    else j["finish_reason"] = nullptr;
}

inline void from_json(const nlohmann::json& j, ChatCompletionChunkChoice& c) {
    j.at("index").get_to(c.index);
    j.at("delta").get_to(c.delta);
    if (j.contains("finish_reason") && !j["finish_reason"].is_null()) {
        c.finish_reason = j["finish_reason"].get<std::string>();
    }
}

// ---------------------------------------------------------------------------
// JSON serialization: ChatCompletionChunk
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ChatCompletionChunk& c) {
    j = nlohmann::json{
        {"id", c.id}, {"object", c.object}, {"created", c.created},
        {"model", c.model}, {"choices", c.choices}
    };
}

inline void from_json(const nlohmann::json& j, ChatCompletionChunk& c) {
    j.at("id").get_to(c.id);
    j.at("object").get_to(c.object);
    j.at("created").get_to(c.created);
    j.at("model").get_to(c.model);
    j.at("choices").get_to(c.choices);
}

// ---------------------------------------------------------------------------
// JSON serialization: Model
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const Model& m) {
    j = nlohmann::json{
        {"id", m.id}, {"object", m.object},
        {"created", m.created}, {"owned_by", m.owned_by}
    };
}

inline void from_json(const nlohmann::json& j, Model& m) {
    j.at("id").get_to(m.id);
    j.at("object").get_to(m.object);
    j.at("created").get_to(m.created);
    j.at("owned_by").get_to(m.owned_by);
}

// ---------------------------------------------------------------------------
// JSON serialization: ModelList
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ModelList& ml) {
    j = nlohmann::json{{"object", ml.object}, {"data", ml.data}};
}

inline void from_json(const nlohmann::json& j, ModelList& ml) {
    j.at("object").get_to(ml.object);
    j.at("data").get_to(ml.data);
}

// ---------------------------------------------------------------------------
// JSON serialization: Transcription
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const Transcription& t) {
    j = nlohmann::json{{"text", t.text}};
}

inline void from_json(const nlohmann::json& j, Transcription& t) {
    j.at("text").get_to(t.text);
}

// ---------------------------------------------------------------------------
// JSON serialization: ImageData
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ImageData& d) {
    j = nlohmann::json::object();
    if (d.url) j["url"] = *d.url;
    if (d.b64_json) j["b64_json"] = *d.b64_json;
    if (d.revised_prompt) j["revised_prompt"] = *d.revised_prompt;
}

inline void from_json(const nlohmann::json& j, ImageData& d) {
    if (j.contains("url") && !j["url"].is_null()) d.url = j["url"].get<std::string>();
    if (j.contains("b64_json") && !j["b64_json"].is_null()) d.b64_json = j["b64_json"].get<std::string>();
    if (j.contains("revised_prompt") && !j["revised_prompt"].is_null()) d.revised_prompt = j["revised_prompt"].get<std::string>();
}

// ---------------------------------------------------------------------------
// JSON serialization: ImageResponse
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const ImageResponse& r) {
    j = nlohmann::json{{"created", r.created}, {"data", r.data}};
}

inline void from_json(const nlohmann::json& j, ImageResponse& r) {
    j.at("created").get_to(r.created);
    j.at("data").get_to(r.data);
}

// ---------------------------------------------------------------------------
// JSON serialization: APIKeyInfo
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const APIKeyInfo& k) {
    j = nlohmann::json{
        {"id", k.id}, {"name", k.name}, {"prefix", k.prefix},
        {"user_id", k.user_id}, {"created_at", k.created_at},
        {"balance_enabled", k.balance_enabled},
        {"balance_sc", k.balance_sc},
        {"balance_limit_sc", k.balance_limit_sc}
    };
    if (k.last_used_at) j["last_used_at"] = *k.last_used_at;
    else j["last_used_at"] = nullptr;
    if (k.rate_limit_rpm) j["rate_limit_rpm"] = *k.rate_limit_rpm;
    else j["rate_limit_rpm"] = nullptr;
    if (k.rate_limit_tpm) j["rate_limit_tpm"] = *k.rate_limit_tpm;
    else j["rate_limit_tpm"] = nullptr;
}

inline void from_json(const nlohmann::json& j, APIKeyInfo& k) {
    j.at("id").get_to(k.id);
    j.at("name").get_to(k.name);
    j.at("prefix").get_to(k.prefix);
    j.at("user_id").get_to(k.user_id);
    j.at("created_at").get_to(k.created_at);
    j.at("balance_enabled").get_to(k.balance_enabled);
    j.at("balance_sc").get_to(k.balance_sc);
    j.at("balance_limit_sc").get_to(k.balance_limit_sc);
    if (j.contains("last_used_at") && !j["last_used_at"].is_null()) {
        k.last_used_at = j["last_used_at"].get<std::string>();
    }
    if (j.contains("rate_limit_rpm") && !j["rate_limit_rpm"].is_null()) {
        k.rate_limit_rpm = j["rate_limit_rpm"].get<int>();
    }
    if (j.contains("rate_limit_tpm") && !j["rate_limit_tpm"].is_null()) {
        k.rate_limit_tpm = j["rate_limit_tpm"].get<int>();
    }
}

// ---------------------------------------------------------------------------
// JSON serialization: APIKeyUsage
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const APIKeyUsage& u) {
    j = nlohmann::json{
        {"total_requests", u.total_requests},
        {"total_tokens_in", u.total_tokens_in},
        {"total_tokens_out", u.total_tokens_out},
        {"total_cost_sc", u.total_cost_sc}
    };
}

inline void from_json(const nlohmann::json& j, APIKeyUsage& u) {
    j.at("total_requests").get_to(u.total_requests);
    j.at("total_tokens_in").get_to(u.total_tokens_in);
    j.at("total_tokens_out").get_to(u.total_tokens_out);
    j.at("total_cost_sc").get_to(u.total_cost_sc);
}

// ---------------------------------------------------------------------------
// JSON serialization: APIKeyDailyUsage
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const APIKeyDailyUsage& u) {
    j = nlohmann::json{
        {"date", u.date}, {"model", u.model}, {"requests", u.requests},
        {"tokens_in", u.tokens_in}, {"tokens_out", u.tokens_out},
        {"cost_sc", u.cost_sc}
    };
}

inline void from_json(const nlohmann::json& j, APIKeyDailyUsage& u) {
    j.at("date").get_to(u.date);
    j.at("model").get_to(u.model);
    j.at("requests").get_to(u.requests);
    j.at("tokens_in").get_to(u.tokens_in);
    j.at("tokens_out").get_to(u.tokens_out);
    j.at("cost_sc").get_to(u.cost_sc);
}

// ---------------------------------------------------------------------------
// JSON serialization: APIKeyList
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const APIKeyList& l) {
    j = nlohmann::json{{"api_keys", l.api_keys}};
}

inline void from_json(const nlohmann::json& j, APIKeyList& l) {
    j.at("api_keys").get_to(l.api_keys);
}

// ---------------------------------------------------------------------------
// JSON serialization: APIKeyDailyUsageList
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const APIKeyDailyUsageList& l) {
    j = nlohmann::json{{"usage", l.usage}};
}

inline void from_json(const nlohmann::json& j, APIKeyDailyUsageList& l) {
    j.at("usage").get_to(l.usage);
}

// ---------------------------------------------------------------------------
// JSON serialization: AccountUsage
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const AccountUsage& u) {
    j = nlohmann::json::object();
    if (u.balance) j["balance"] = *u.balance;
    if (u.total_spent) j["total_spent"] = *u.total_spent;
    if (u.total_requests) j["total_requests"] = *u.total_requests;
}

inline void from_json(const nlohmann::json& j, AccountUsage& u) {
    if (j.contains("balance") && !j["balance"].is_null()) {
        u.balance = j["balance"].get<double>();
    }
    if (j.contains("total_spent") && !j["total_spent"].is_null()) {
        u.total_spent = j["total_spent"].get<double>();
    }
    if (j.contains("total_requests") && !j["total_requests"].is_null()) {
        u.total_requests = j["total_requests"].get<int>();
    }
}

} // namespace simplellm
