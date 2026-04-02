#include <ssrag/mcp_server.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Minimal JSON handling — no external dependency.
// MCP messages are small; hand-parsing is fine.
namespace {

std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Extract a string value for a given key from a JSON object string.
// Extremely basic — works for flat objects with string values.
std::string json_get_string(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    ++pos;
    std::string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            ++pos;
            switch (json[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default: result += json[pos]; break;
            }
        } else {
            result += json[pos];
        }
        ++pos;
    }
    return result;
}

// Extract an integer value for a given key.
int json_get_int(const std::string& json, const std::string& key, int def = 0) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return def;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    try { return std::stoi(json.substr(pos)); } catch (...) { return def; }
}

// Extract a nested JSON object as a raw string.
std::string json_get_object(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "{}";
    pos = json.find('{', pos + needle.size());
    if (pos == std::string::npos) return "{}";
    int depth = 0;
    size_t start = pos;
    for (; pos < json.size(); ++pos) {
        if (json[pos] == '{') ++depth;
        else if (json[pos] == '}') { --depth; if (depth == 0) return json.substr(start, pos - start + 1); }
    }
    return "{}";
}

} // anonymous namespace

namespace ssrag {

MCPServer::MCPServer()
    : config_()
    , engine_(config_.fold_params) {}

MCPServer::MCPServer(Config config)
    : config_(std::move(config))
    , engine_(config_.fold_params) {}

int MCPServer::run() {
    if (!engine_.init()) {
        std::cerr << "Failed to initialize IOT index\n";
        return 1;
    }

    load_state();

    // MCP stdio transport: read Content-Length headers, then JSON body.
    std::string line;
    while (std::getline(std::cin, line)) {
        // Parse Content-Length header.
        int content_length = 0;
        if (line.find("Content-Length:") != std::string::npos) {
            content_length = std::stoi(line.substr(line.find(':') + 1));
            // Read blank line.
            std::getline(std::cin, line);
            // Read body.
            std::string body(content_length, '\0');
            std::cin.read(body.data(), content_length);

            // Route JSON-RPC.
            std::string method = json_get_string(body, "method");
            std::string id = json_get_string(body, "id");
            // Handle numeric IDs too.
            if (id.empty()) {
                int num_id = json_get_int(body, "id", -1);
                if (num_id >= 0) id = std::to_string(num_id);
            }

            std::string response;
            if (method == "initialize") {
                response = handle_initialize(id);
            } else if (method == "tools/list") {
                response = handle_tools_list(id);
            } else if (method == "tools/call") {
                auto params = json_get_object(body, "params");
                std::string name = json_get_string(params, "name");
                std::string arguments = json_get_object(params, "arguments");
                response = handle_tool_call(id, name, arguments);
            } else if (method == "notifications/initialized") {
                continue;  // No response needed.
            } else {
                response = json_rpc_error(id, -32601, "Method not found: " + method);
            }

            // Write response with Content-Length header.
            std::cout << "Content-Length: " << response.size() << "\r\n\r\n" << response;
            std::cout.flush();
        }
    }

    save_state();
    return 0;
}

std::string MCPServer::handle_initialize(const std::string& id) {
    return json_rpc_response(id, R"({
        "protocolVersion": "2024-11-05",
        "capabilities": { "tools": {} },
        "serverInfo": {
            "name": "ssrag",
            "version": "0.1.0"
        }
    })");
}

std::string MCPServer::handle_tools_list(const std::string& id) {
    return json_rpc_response(id, R"JSON({
        "tools": [
            {
                "name": "ssrag_ingest",
                "description": "Ingest documents into the self-sharpening RAG index. Each document becomes a dimension in the search space, improving ALL future searches.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "File or directory path to ingest"
                        }
                    },
                    "required": ["path"]
                }
            },
            {
                "name": "ssrag_search",
                "description": "Search the self-sharpening RAG index. Returns top-k most relevant document chunks with text content and distance scores.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "query": {
                            "type": "string",
                            "description": "Search query text"
                        },
                        "k": {
                            "type": "integer",
                            "description": "Number of results (default 10)"
                        }
                    },
                    "required": ["query"]
                }
            },
            {
                "name": "ssrag_status",
                "description": "Get index statistics: total chunks, effective dimensionality, estimated contrast ratio.",
                "inputSchema": {
                    "type": "object",
                    "properties": {}
                }
            }
        ]
    })JSON");
}

std::string MCPServer::handle_tool_call(const std::string& id, const std::string& name,
                                         const std::string& arguments) {
    std::string content;
    if (name == "ssrag_ingest") {
        std::string path = json_get_string(arguments, "path");
        content = do_ingest(path);
    } else if (name == "ssrag_search") {
        std::string query = json_get_string(arguments, "query");
        int k = json_get_int(arguments, "k", 10);
        content = do_search(query, static_cast<uint32_t>(k));
    } else if (name == "ssrag_status") {
        content = do_status();
    } else {
        return json_rpc_error(id, -32602, "Unknown tool: " + name);
    }

    return json_rpc_response(id, R"({"content": [{"type": "text", "text": ")" +
                             escape_json(content) + R"("}]})");
}

std::string MCPServer::do_ingest(const std::string& path_str) {
    std::filesystem::path path(path_str);
    if (!std::filesystem::exists(path)) {
        return "Error: path does not exist: " + path_str;
    }

    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                if (ext == ".txt" || ext == ".md" || ext == ".csv" || ext == ".json" ||
                    ext == ".tex" || ext == ".bib" || ext == ".xml" || ext == ".html" ||
                    ext == ".py" || ext == ".cpp" || ext == ".hpp" || ext == ".c" ||
                    ext == ".h" || ext == ".rs" || ext == ".go" || ext == ".java") {
                    files.push_back(entry.path());
                }
            }
        }
    } else {
        files.push_back(path);
    }

    uint64_t total_chunks = 0;
    uint32_t total_edges = 0;

    for (const auto& file : files) {
        std::ifstream in(file);
        if (!in) continue;
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());

        auto chunks = chunk_text(content);
        for (auto& tc : chunks) {
            ChunkMeta meta;
            meta.source_path = file.string();
            meta.byte_offset = tc.byte_offset;
            meta.byte_length = tc.byte_length;

            auto result = engine_.ingest(store_, std::move(tc.text), std::move(meta));
            total_chunks++;
            total_edges += result.edges_created;
        }
    }

    save_state();

    std::ostringstream oss;
    oss << "Ingested " << total_chunks << " chunks from " << files.size() << " files.\n"
        << "Fold edges created: " << total_edges << "\n"
        << "Effective dimensionality: " << engine_.effective_dim() << "\n"
        << "Estimated contrast ratio: " << engine_.estimated_contrast() << ":1";
    return oss.str();
}

std::string MCPServer::do_search(const std::string& query, uint32_t k) {
    auto hits = engine_.search(store_, query, k);
    if (hits.empty()) {
        return "No results found. Index may be empty (use ssrag_ingest first).";
    }

    std::ostringstream oss;
    oss << hits.size() << " results:\n\n";
    for (size_t i = 0; i < hits.size(); ++i) {
        const Chunk* chunk = store_.get(hits[i].id);
        if (!chunk) continue;
        oss << "--- Result " << (i + 1) << " (distance: " << hits[i].distance << ") ---\n"
            << "Source: " << chunk->meta.source_path
            << " [offset " << chunk->meta.byte_offset << "]\n"
            << chunk->text << "\n\n";
    }
    return oss.str();
}

std::string MCPServer::do_status() {
    std::ostringstream oss;
    oss << "SSRAG Index Status:\n"
        << "  Total chunks: " << store_.size() << "\n"
        << "  Effective dimensionality: " << engine_.effective_dim() << "\n"
        << "  Estimated contrast ratio: " << engine_.estimated_contrast() << ":1\n"
        << "  Base dimensions: " << BASE_DIMS;
    return oss.str();
}

std::vector<MCPServer::TextChunk> MCPServer::chunk_text(std::string_view text) const {
    std::vector<TextChunk> chunks;
    if (text.empty()) return chunks;

    uint64_t offset = 0;
    while (offset < text.size()) {
        uint64_t end = std::min(offset + static_cast<uint64_t>(config_.chunk_size),
                                static_cast<uint64_t>(text.size()));

        // Try to break at a paragraph or sentence boundary.
        if (end < text.size()) {
            // Look for paragraph break.
            auto para = text.rfind("\n\n", end);
            if (para != std::string_view::npos && para > offset + config_.chunk_size / 2) {
                end = para + 2;
            } else {
                // Look for sentence end.
                auto sent = text.rfind(". ", end);
                if (sent != std::string_view::npos && sent > offset + config_.chunk_size / 2) {
                    end = sent + 2;
                }
            }
        }

        TextChunk tc;
        tc.text = std::string(text.substr(offset, end - offset));
        tc.byte_offset = offset;
        tc.byte_length = end - offset;
        chunks.push_back(std::move(tc));

        // Advance with overlap.
        if (end >= text.size()) break;
        offset = end > config_.chunk_overlap ? end - config_.chunk_overlap : end;
    }

    return chunks;
}

void MCPServer::save_state() {
    std::filesystem::create_directories(config_.data_dir);
    store_.save(config_.data_dir / "store.bin");
    engine_.save_index(config_.data_dir / "index.bin");
}

void MCPServer::load_state() {
    auto store_path = config_.data_dir / "store.bin";
    auto index_path = config_.data_dir / "index.bin";
    if (std::filesystem::exists(store_path)) {
        store_.load(store_path);
    }
    if (std::filesystem::exists(index_path)) {
        engine_.load_index(index_path);
    }
}

std::string MCPServer::json_rpc_response(const std::string& id, const std::string& result) {
    // Try numeric ID first.
    bool numeric = !id.empty() && std::all_of(id.begin(), id.end(),
                                               [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
    std::string id_str = numeric ? id : ("\"" + escape_json(id) + "\"");
    return R"({"jsonrpc":"2.0","id":)" + id_str + R"(,"result":)" + result + "}";
}

std::string MCPServer::json_rpc_error(const std::string& id, int code, const std::string& msg) {
    bool numeric = !id.empty() && std::all_of(id.begin(), id.end(),
                                               [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
    std::string id_str = numeric ? id : ("\"" + escape_json(id) + "\"");
    return R"({"jsonrpc":"2.0","id":)" + id_str +
           R"(,"error":{"code":)" + std::to_string(code) +
           R"(,"message":")" + escape_json(msg) + R"("}})";
}

} // namespace ssrag
