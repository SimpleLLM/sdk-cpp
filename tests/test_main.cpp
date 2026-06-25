#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "simplellm/types.hpp"
#include "simplellm/sse_parser.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace simplellm;

// ---------------------------------------------------------------------------
// Test 1: ChatCompletion JSON round-trip
// ---------------------------------------------------------------------------

TEST_CASE("ChatCompletion JSON round-trip") {
    const std::string raw = R"({
        "id": "chatcmpl-abc123",
        "object": "chat.completion",
        "created": 1700000000,
        "model": "gemma-3-27b",
        "choices": [
            {
                "index": 0,
                "message": {
                    "role": "assistant",
                    "content": "Hello, world!"
                },
                "finish_reason": "stop"
            }
        ],
        "usage": {
            "prompt_tokens": 10,
            "completion_tokens": 5,
            "total_tokens": 15
        }
    })";

    auto j = nlohmann::json::parse(raw);
    ChatCompletion cc = j.get<ChatCompletion>();

    CHECK(cc.id == "chatcmpl-abc123");
    CHECK(cc.object == "chat.completion");
    CHECK(cc.created == 1700000000LL);
    CHECK(cc.model == "gemma-3-27b");
    REQUIRE(cc.choices.size() == 1);
    CHECK(cc.choices[0].index == 0);
    CHECK(cc.choices[0].message.role == "assistant");
    REQUIRE(cc.choices[0].message.content.has_value());
    CHECK(cc.choices[0].message.content.value() == "Hello, world!");
    REQUIRE(cc.choices[0].finish_reason.has_value());
    CHECK(cc.choices[0].finish_reason.value() == "stop");
    REQUIRE(cc.usage.has_value());
    CHECK(cc.usage->prompt_tokens == 10);
    CHECK(cc.usage->completion_tokens == 5);
    CHECK(cc.usage->total_tokens == 15);

    // Serialize back and re-parse to verify round-trip
    nlohmann::json j2 = cc;
    ChatCompletion cc2 = j2.get<ChatCompletion>();
    CHECK(cc2.id == cc.id);
    CHECK(cc2.model == cc.model);
    CHECK(cc2.choices[0].message.content.value() == "Hello, world!");
}

// ---------------------------------------------------------------------------
// Test 2: ChatCompletionChunk JSON deserialization
// ---------------------------------------------------------------------------

TEST_CASE("ChatCompletionChunk JSON deserialization") {
    const std::string raw = R"({
        "id": "chatcmpl-chunk1",
        "object": "chat.completion.chunk",
        "created": 1700000001,
        "model": "gemma-3-27b",
        "choices": [
            {
                "index": 0,
                "delta": {
                    "role": "assistant",
                    "content": "Hello"
                },
                "finish_reason": null
            }
        ]
    })";

    auto j = nlohmann::json::parse(raw);
    ChatCompletionChunk chunk = j.get<ChatCompletionChunk>();

    CHECK(chunk.id == "chatcmpl-chunk1");
    CHECK(chunk.object == "chat.completion.chunk");
    CHECK(chunk.created == 1700000001LL);
    CHECK(chunk.model == "gemma-3-27b");
    REQUIRE(chunk.choices.size() == 1);
    CHECK(chunk.choices[0].index == 0);
    REQUIRE(chunk.choices[0].delta.role.has_value());
    CHECK(chunk.choices[0].delta.role.value() == "assistant");
    REQUIRE(chunk.choices[0].delta.content.has_value());
    CHECK(chunk.choices[0].delta.content.value() == "Hello");
    CHECK(!chunk.choices[0].finish_reason.has_value());
}

// ---------------------------------------------------------------------------
// Test 3: ModelList JSON deserialization
// ---------------------------------------------------------------------------

TEST_CASE("ModelList JSON deserialization") {
    const std::string raw = R"({
        "object": "list",
        "data": [
            {
                "id": "gemma-3-27b",
                "object": "model",
                "created": 1699000000,
                "owned_by": "simplellm"
            },
            {
                "id": "deepseek-v3",
                "object": "model",
                "created": 1700000000,
                "owned_by": "simplellm"
            }
        ]
    })";

    auto j = nlohmann::json::parse(raw);
    ModelList ml = j.get<ModelList>();

    CHECK(ml.object == "list");
    REQUIRE(ml.data.size() == 2);
    CHECK(ml.data[0].id == "gemma-3-27b");
    CHECK(ml.data[0].object == "model");
    CHECK(ml.data[0].created == 1699000000LL);
    CHECK(ml.data[0].owned_by == "simplellm");
    CHECK(ml.data[1].id == "deepseek-v3");
}

// ---------------------------------------------------------------------------
// Test 4: APIKeyInfo JSON deserialization — optionals present and absent
// ---------------------------------------------------------------------------

TEST_CASE("APIKeyInfo JSON deserialization - all fields") {
    const std::string raw = R"({
        "id": "key_abc123",
        "name": "My API Key",
        "prefix": "sk-simplellm-abc",
        "user_id": "user_xyz",
        "created_at": "2024-01-01T00:00:00Z",
        "last_used_at": "2024-06-01T12:00:00Z",
        "balance_enabled": true,
        "balance_sc": 100.5,
        "balance_limit_sc": 500.0,
        "rate_limit_rpm": 60,
        "rate_limit_tpm": 100000
    })";

    auto j = nlohmann::json::parse(raw);
    APIKeyInfo k = j.get<APIKeyInfo>();

    CHECK(k.id == "key_abc123");
    CHECK(k.name == "My API Key");
    CHECK(k.prefix == "sk-simplellm-abc");
    CHECK(k.user_id == "user_xyz");
    CHECK(k.created_at == "2024-01-01T00:00:00Z");
    REQUIRE(k.last_used_at.has_value());
    CHECK(k.last_used_at.value() == "2024-06-01T12:00:00Z");
    CHECK(k.balance_enabled == true);
    CHECK(k.balance_sc == doctest::Approx(100.5));
    CHECK(k.balance_limit_sc == doctest::Approx(500.0));
    REQUIRE(k.rate_limit_rpm.has_value());
    CHECK(k.rate_limit_rpm.value() == 60);
    REQUIRE(k.rate_limit_tpm.has_value());
    CHECK(k.rate_limit_tpm.value() == 100000);
}

TEST_CASE("APIKeyInfo JSON deserialization - optional fields absent/null") {
    const std::string raw = R"({
        "id": "key_def456",
        "name": "Basic Key",
        "prefix": "sk-simplellm-def",
        "user_id": "user_abc",
        "created_at": "2024-03-15T10:00:00Z",
        "last_used_at": null,
        "balance_enabled": false,
        "balance_sc": 0.0,
        "balance_limit_sc": 0.0,
        "rate_limit_rpm": null,
        "rate_limit_tpm": null
    })";

    auto j = nlohmann::json::parse(raw);
    APIKeyInfo k = j.get<APIKeyInfo>();

    CHECK(k.id == "key_def456");
    CHECK(!k.last_used_at.has_value());
    CHECK(k.balance_enabled == false);
    CHECK(!k.rate_limit_rpm.has_value());
    CHECK(!k.rate_limit_tpm.has_value());
}

// ---------------------------------------------------------------------------
// Test 5: SSE parsing
// ---------------------------------------------------------------------------

TEST_CASE("parse_sse_data_line: empty line returns nullopt") {
    CHECK(!parse_sse_data_line("").has_value());
}

TEST_CASE("parse_sse_data_line: non-data line returns nullopt") {
    CHECK(!parse_sse_data_line("event: message").has_value());
    CHECK(!parse_sse_data_line(": comment").has_value());
}

TEST_CASE("parse_sse_data_line: [DONE] returns nullopt") {
    CHECK(!parse_sse_data_line("data: [DONE]").has_value());
}

TEST_CASE("parse_sse_data_line: valid JSON data line") {
    std::string line = R"(data: {"id":"chunk1","object":"chat.completion.chunk","created":1700000000,"model":"test","choices":[{"index":0,"delta":{"content":"Hi"},"finish_reason":null}]})";
    auto result = parse_sse_data_line(line);
    REQUIRE(result.has_value());
    CHECK(result->at("id").get<std::string>() == "chunk1");
}

TEST_CASE("parse_sse_chunks: full SSE stream") {
    // Build a realistic SSE body
    const std::string sse_body =
        "data: {\"id\":\"chunk1\",\"object\":\"chat.completion.chunk\",\"created\":1700000000,\"model\":\"test\",\"choices\":[{\"index\":0,\"delta\":{\"role\":\"assistant\",\"content\":\"\"},\"finish_reason\":null}]}\n"
        "\n"
        "data: {\"id\":\"chunk1\",\"object\":\"chat.completion.chunk\",\"created\":1700000000,\"model\":\"test\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\"Hello\"},\"finish_reason\":null}]}\n"
        "\n"
        "data: {\"id\":\"chunk1\",\"object\":\"chat.completion.chunk\",\"created\":1700000000,\"model\":\"test\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\" world\"},\"finish_reason\":null}]}\n"
        "\n"
        "data: {\"id\":\"chunk1\",\"object\":\"chat.completion.chunk\",\"created\":1700000000,\"model\":\"test\",\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"stop\"}]}\n"
        "\n"
        "data: [DONE]\n";

    auto chunks = parse_sse_chunks(sse_body);

    REQUIRE(chunks.size() == 4);

    // First chunk: role init
    CHECK(chunks[0].choices[0].delta.role.value() == "assistant");
    CHECK(chunks[0].choices[0].delta.content.value() == "");

    // Second chunk: "Hello"
    CHECK(chunks[1].choices[0].delta.content.value() == "Hello");

    // Third chunk: " world"
    CHECK(chunks[2].choices[0].delta.content.value() == " world");

    // Fourth chunk: finish
    CHECK(chunks[3].choices[0].finish_reason.value() == "stop");
}

TEST_CASE("parse_sse_chunks: handles CRLF line endings") {
    const std::string sse_body =
        "data: {\"id\":\"c1\",\"object\":\"chat.completion.chunk\",\"created\":1700000000,\"model\":\"m\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\"A\"},\"finish_reason\":null}]}\r\n"
        "\r\n"
        "data: [DONE]\r\n";

    auto chunks = parse_sse_chunks(sse_body);
    REQUIRE(chunks.size() == 1);
    CHECK(chunks[0].choices[0].delta.content.value() == "A");
}

TEST_CASE("parse_sse_chunks: stops at [DONE]") {
    // Content after [DONE] should be ignored
    const std::string sse_body =
        "data: {\"id\":\"c1\",\"object\":\"chat.completion.chunk\",\"created\":1700000000,\"model\":\"m\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\"X\"},\"finish_reason\":null}]}\n"
        "data: [DONE]\n"
        "data: {\"id\":\"c2\",\"object\":\"chat.completion.chunk\",\"created\":1700000001,\"model\":\"m\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\"Y\"},\"finish_reason\":null}]}\n";

    auto chunks = parse_sse_chunks(sse_body);
    REQUIRE(chunks.size() == 1);
    CHECK(chunks[0].choices[0].delta.content.value() == "X");
}

TEST_CASE("parse_sse_chunks: empty body returns empty vector") {
    CHECK(parse_sse_chunks("").empty());
}

// ---------------------------------------------------------------------------
// Test 6: Error type
// ---------------------------------------------------------------------------

TEST_CASE("Error is catchable as std::runtime_error") {
    try {
        throw Error("test error", 404, "not_found");
    } catch (const std::runtime_error& e) {
        CHECK(std::string(e.what()) == "test error");
    } catch (...) {
        FAIL("Should be caught as runtime_error");
    }
}

TEST_CASE("Error carries status and code") {
    try {
        throw Error("forbidden", 403, "forbidden");
    } catch (const Error& e) {
        CHECK(e.status == 403);
        CHECK(e.code == "forbidden");
        CHECK(std::string(e.what()) == "forbidden");
    }
}

// ---------------------------------------------------------------------------
// Test 7: ChatCompletionRequest serialization (optional fields)
// ---------------------------------------------------------------------------

TEST_CASE("ChatCompletionRequest to_json with optional fields") {
    ChatCompletionRequest req;
    req.model = "gemma-3-27b";
    req.messages.push_back({"user", "Hello!", std::nullopt, std::nullopt, std::nullopt});
    req.temperature = 0.7;
    req.max_tokens = 256;

    nlohmann::json j = req;
    CHECK(j["model"].get<std::string>() == "gemma-3-27b");
    CHECK(j["messages"][0]["role"].get<std::string>() == "user");
    CHECK(j["temperature"].get<double>() == doctest::Approx(0.7));
    CHECK(j["max_tokens"].get<int>() == 256);
    // top_p was not set, should not be present
    CHECK(!j.contains("top_p"));
    CHECK(!j.contains("tools"));
}
