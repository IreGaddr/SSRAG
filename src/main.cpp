#include <ssrag/mcp_server.hpp>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    ssrag::MCPServer::Config config;

    // Parse optional --data-dir argument.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) {
            config.data_dir = argv[++i];
        } else if (arg == "--chunk-size" && i + 1 < argc) {
            config.chunk_size = static_cast<uint32_t>(std::stoul(argv[++i]));
        }
    }

    ssrag::MCPServer server(config);
    return server.run();
}
