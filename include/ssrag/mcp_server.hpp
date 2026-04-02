#ifndef SSRAG_MCP_SERVER_HPP
#define SSRAG_MCP_SERVER_HPP

#include <ssrag/lambda_fold.hpp>
#include <ssrag/document_store.hpp>
#include <filesystem>
#include <string>

namespace ssrag {

// Minimal MCP (Model Context Protocol) server over stdio.
// Implements JSON-RPC 2.0 with three tools:
//   ssrag_ingest  — ingest files/directories
//   ssrag_search  — search the index
//   ssrag_status  — index stats
class MCPServer {
public:
    struct Config {
        std::filesystem::path data_dir{"ssrag_data"};
        uint32_t chunk_size = 1024;
        uint32_t chunk_overlap = 128;
        FoldParams fold_params{};
    };

    MCPServer();
    explicit MCPServer(Config config);

    // Run the server (blocks, reads stdin, writes stdout).
    int run();

private:
    Config config_;
    DocumentStore store_;
    LambdaFoldEngine engine_;

    // JSON-RPC handlers.
    std::string handle_initialize(const std::string& id);
    std::string handle_tools_list(const std::string& id);
    std::string handle_tool_call(const std::string& id, const std::string& name,
                                 const std::string& arguments);

    // Tool implementations.
    std::string do_ingest(const std::string& path_str);
    std::string do_search(const std::string& query, uint32_t k);
    std::string do_status();

    // Chunking.
    struct TextChunk {
        std::string text;
        uint64_t byte_offset;
        uint64_t byte_length;
    };
    std::vector<TextChunk> chunk_text(std::string_view text) const;

    // Persistence.
    void save_state();
    void load_state();

    // JSON helpers.
    std::string json_rpc_response(const std::string& id, const std::string& result);
    std::string json_rpc_error(const std::string& id, int code, const std::string& msg);
};

} // namespace ssrag

#endif // SSRAG_MCP_SERVER_HPP
