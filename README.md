# SimpleLLM C++ SDK

C++17 SDK for the [SimpleLLM](https://simplellm.eu) API — an EU-hosted LLM inference service with an OpenAI-compatible interface.

## Requirements

- C++17 compiler (GCC 12+, Clang 14+)
- CMake 3.16+
- OpenSSL 3.x dev headers

Dependencies (fetched automatically via CMake FetchContent):
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) v0.18.5
- [nlohmann/json](https://github.com/nlohmann/json) v3.11.3

## Build

```bash
cmake -B build
cmake --build build -j
```

With tests:

```bash
cmake -B build -DSIMPLELLM_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Quick Start

```cpp
#include <simplellm/simplellm.hpp>
#include <iostream>

int main() {
    // API key from env SIMPLELLM_API_KEY, or pass directly
    simplellm::Client client("sk-your-api-key");

    simplellm::ChatCompletionRequest req;
    req.model = "Qwen3-Coder-30B-A3B-Instruct";
    req.messages.push_back({"user", "Hello!", {}, {}, {}});
    req.temperature = 0.7;

    auto response = client.chat_completions_create(req);
    std::cout << response.choices[0].message.content.value_or("") << "\n";

    return 0;
}
```

## Streaming

```cpp
simplellm::ChatCompletionRequest req;
req.model = "Qwen3-Coder-30B-A3B-Instruct";
req.messages.push_back({"user", "Tell me a story", {}, {}, {}});

client.chat_completions_stream(req, [](const simplellm::ChatCompletionChunk& chunk) -> bool {
    if (!chunk.choices.empty() && chunk.choices[0].delta.content) {
        std::cout << *chunk.choices[0].delta.content << std::flush;
    }
    return true; // return false to abort
});
std::cout << "\n";
```

## Environment Variables

| Variable | Default | Description |
|---|---|---|
| `SIMPLELLM_API_KEY` | — | API key (required if not passed to constructor) |
| `SIMPLELLM_BASE_URL` | `https://api.simplellm.eu` | API base URL |

## Namespace

All types and classes live in the `simplellm` namespace.
