// STRING database ingestion pipeline for SSRAG.
//
// Reads STRING yeast data files and constructs one text document per protein,
// combining: annotation, aliases, enrichment terms, and top interaction partners.
// Then ingests each document through the LambdaFoldEngine.
//
// Usage: ssrag-ingest-string --data-dir <path-to-string-files> [--out-dir <ssrag-data>]

#include <ssrag/lambda_fold.hpp>
#include <ssrag/document_store.hpp>
#include <ssrag/fingerprint.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// ---- Data structures for STRING tables ----

struct ProteinInfo {
    std::string string_id;     // e.g. "4932.YAL001C"
    std::string name;          // e.g. "TFC3"
    uint32_t size = 0;         // amino acids
    std::string annotation;    // free text
};

struct Interaction {
    std::string partner_id;
    uint16_t neighborhood = 0;
    uint16_t fusion = 0;
    uint16_t cooccurrence = 0;
    uint16_t coexpression = 0;
    uint16_t experimental = 0;
    uint16_t database = 0;
    uint16_t textmining = 0;
    uint16_t combined = 0;
};

struct EnrichmentTerm {
    std::string category;
    std::string term_id;
    std::string description;
};

// ---- Parsers ----

std::unordered_map<std::string, ProteinInfo> parse_protein_info(const fs::path& path) {
    std::unordered_map<std::string, ProteinInfo> out;
    std::ifstream in(path);
    std::string line;
    std::getline(in, line); // skip header

    while (std::getline(in, line)) {
        std::istringstream ss(line);
        ProteinInfo p;
        std::string size_str;
        ss >> p.string_id >> p.name >> size_str;
        p.size = static_cast<uint32_t>(std::stoul(size_str));
        // Rest of line is annotation (tab-separated, take everything after 3rd field).
        auto tab3 = line.find('\t');
        tab3 = line.find('\t', tab3 + 1);
        tab3 = line.find('\t', tab3 + 1);
        if (tab3 != std::string::npos) {
            p.annotation = line.substr(tab3 + 1);
        }
        out[p.string_id] = std::move(p);
    }
    return out;
}

// Parse interactions, keeping only top-N partners per protein by combined score.
std::unordered_map<std::string, std::vector<Interaction>>
parse_interactions(const fs::path& path, uint32_t top_n = 50) {
    std::unordered_map<std::string, std::vector<Interaction>> out;
    std::ifstream in(path);
    std::string line;
    std::getline(in, line); // skip header

    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string p1, p2;
        Interaction ix;
        ss >> p1 >> p2
           >> ix.neighborhood >> ix.fusion >> ix.cooccurrence
           >> ix.coexpression >> ix.experimental >> ix.database
           >> ix.textmining >> ix.combined;
        ix.partner_id = p2;
        out[p1].push_back(ix);
    }

    // Keep only top-N per protein.
    for (auto& [pid, ixs] : out) {
        std::sort(ixs.begin(), ixs.end(), [](const Interaction& a, const Interaction& b) {
            return a.combined > b.combined;
        });
        if (ixs.size() > top_n) {
            ixs.resize(top_n);
        }
    }
    return out;
}

std::unordered_map<std::string, std::vector<EnrichmentTerm>>
parse_enrichment(const fs::path& path) {
    std::unordered_map<std::string, std::vector<EnrichmentTerm>> out;
    std::ifstream in(path);
    std::string line;
    std::getline(in, line); // skip header

    while (std::getline(in, line)) {
        // Format: string_protein_id \t category \t term \t description
        size_t t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        size_t t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        size_t t3 = line.find('\t', t2 + 1);
        if (t3 == std::string::npos) continue;

        std::string pid = line.substr(0, t1);
        EnrichmentTerm et;
        et.category = line.substr(t1 + 1, t2 - t1 - 1);
        et.term_id = line.substr(t2 + 1, t3 - t2 - 1);
        et.description = line.substr(t3 + 1);
        out[pid].push_back(std::move(et));
    }
    return out;
}

// ---- Document builder ----

std::string build_protein_document(
    const ProteinInfo& info,
    const std::vector<Interaction>& interactions,
    const std::vector<EnrichmentTerm>& terms,
    const std::unordered_map<std::string, ProteinInfo>& all_proteins)
{
    std::ostringstream doc;

    // Header: name and annotation.
    doc << info.name << " (" << info.string_id << ") "
        << info.size << " amino acids\n\n";

    if (!info.annotation.empty()) {
        doc << info.annotation << "\n\n";
    }

    // Enrichment terms grouped by category.
    if (!terms.empty()) {
        std::unordered_map<std::string, std::vector<const EnrichmentTerm*>> by_cat;
        for (const auto& t : terms) by_cat[t.category].push_back(&t);

        for (const auto& [cat, cat_terms] : by_cat) {
            doc << cat << ": ";
            for (size_t i = 0; i < cat_terms.size(); ++i) {
                if (i > 0) doc << ", ";
                doc << cat_terms[i]->description;
            }
            doc << "\n";
        }
        doc << "\n";
    }

    // Top interaction partners with evidence breakdown.
    if (!interactions.empty()) {
        doc << "Interaction partners (top " << interactions.size() << "):\n";
        for (const auto& ix : interactions) {
            auto it = all_proteins.find(ix.partner_id);
            std::string partner_name = (it != all_proteins.end()) ?
                it->second.name : ix.partner_id;

            doc << "  " << partner_name << " (score " << ix.combined << ":";
            if (ix.experimental > 0)  doc << " exp=" << ix.experimental;
            if (ix.database > 0)      doc << " db=" << ix.database;
            if (ix.textmining > 0)    doc << " txt=" << ix.textmining;
            if (ix.coexpression > 0)  doc << " coex=" << ix.coexpression;
            if (ix.cooccurrence > 0)  doc << " coocc=" << ix.cooccurrence;
            if (ix.neighborhood > 0)  doc << " neigh=" << ix.neighborhood;
            if (ix.fusion > 0)        doc << " fus=" << ix.fusion;
            doc << ")\n";
        }
    }

    return doc.str();
}

// ---- Main ----

int main(int argc, char* argv[]) {
    fs::path data_dir = "/home/ire/code/ssrag/data/string_yeast";
    fs::path out_dir = "ssrag_data";
    std::string organism = "4932";  // default: yeast
    uint32_t top_partners = 50;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) data_dir = argv[++i];
        else if (arg == "--out-dir" && i + 1 < argc) out_dir = argv[++i];
        else if (arg == "--organism" && i + 1 < argc) organism = argv[++i];
        else if (arg == "--top-partners" && i + 1 < argc)
            top_partners = static_cast<uint32_t>(std::stoul(argv[++i]));
    }

    // Check for decompressed files, decompress if needed.
    auto ensure_decompressed = [&](const std::string& basename) -> fs::path {
        fs::path gz = data_dir / (basename + ".gz");
        fs::path txt = data_dir / basename;
        if (fs::exists(txt)) return txt;
        if (!fs::exists(gz)) {
            std::cerr << "Missing: " << gz << "\n";
            return {};
        }
        std::cerr << "Decompressing " << gz.filename() << "...\n";
        std::string cmd = "gzip -dk \"" + gz.string() + "\"";
        if (std::system(cmd.c_str()) != 0) {
            std::cerr << "Failed to decompress " << gz << "\n";
            return {};
        }
        return txt;
    };

    auto info_path = ensure_decompressed(organism + ".protein.info.v12.0.txt");
    auto links_path = ensure_decompressed(organism + ".protein.links.detailed.v12.0.txt");
    auto enrich_path = ensure_decompressed(organism + ".protein.enrichment.terms.v12.0.txt");

    if (info_path.empty() || links_path.empty() || enrich_path.empty()) {
        std::cerr << "Missing required data files in " << data_dir << "\n";
        return 1;
    }

    // Parse all data.
    std::cerr << "Parsing protein info...\n";
    auto proteins = parse_protein_info(info_path);
    std::cerr << "  " << proteins.size() << " proteins\n";

    std::cerr << "Parsing interactions (keeping top " << top_partners << " per protein)...\n";
    auto interactions = parse_interactions(links_path, top_partners);
    std::cerr << "  " << interactions.size() << " proteins with interactions\n";

    std::cerr << "Parsing enrichment terms...\n";
    auto enrichment = parse_enrichment(enrich_path);
    std::cerr << "  " << enrichment.size() << " proteins with terms\n";

    // Build documents.
    std::cerr << "Building protein documents...\n";
    struct ProteinDoc {
        std::string string_id;
        std::string text;
    };
    std::vector<ProteinDoc> docs;
    docs.reserve(proteins.size());

    for (const auto& [pid, info] : proteins) {
        auto ix_it = interactions.find(pid);
        auto en_it = enrichment.find(pid);

        const auto& ixs = (ix_it != interactions.end()) ?
            ix_it->second : std::vector<Interaction>{};
        const auto& terms = (en_it != enrichment.end()) ?
            en_it->second : std::vector<EnrichmentTerm>{};

        std::string text = build_protein_document(info, ixs, terms, proteins);
        docs.push_back({pid, std::move(text)});
    }

    // Sort by string_id for deterministic ingestion order.
    std::sort(docs.begin(), docs.end(), [](const ProteinDoc& a, const ProteinDoc& b) {
        return a.string_id < b.string_id;
    });

    std::cerr << "Built " << docs.size() << " documents\n";

    // Sample a document to stderr for sanity check.
    if (!docs.empty()) {
        // Pick a well-known protein if available.
        const ProteinDoc* sample = &docs[0];
        for (const auto& d : docs) {
            if (d.text.find("COX1") != std::string::npos ||
                d.text.find("CDC28") != std::string::npos ||
                d.text.find("ACT1") != std::string::npos) {
                sample = &d;
                break;
            }
        }
        std::cerr << "\n--- Sample document (" << sample->string_id << ") ---\n"
                  << sample->text.substr(0, 1500) << "\n---\n\n";
    }

    // Initialize SSRAG engine.
    std::cerr << "Initializing SSRAG engine...\n";
    ssrag::FoldParams fold_params;
    fold_params.fold_k = 200;
    fold_params.fold_threshold = 0.02f;
    ssrag::LambdaFoldEngine engine(fold_params);
    if (!engine.init()) {
        std::cerr << "Failed to initialize IOT index\n";
        return 1;
    }

    ssrag::DocumentStore store;

    // Load existing state if present.
    fs::create_directories(out_dir);
    auto store_path = out_dir / "store.bin";
    auto index_path = out_dir / "index.bin";
    if (fs::exists(store_path)) {
        std::cerr << "Loading existing index...\n";
        store.load(store_path);
        engine.load_index(index_path);
        std::cerr << "  Loaded " << store.size() << " existing chunks\n";
    }

    // Ingest.
    std::cerr << "Ingesting " << docs.size() << " protein documents...\n";
    auto t_start = std::chrono::steady_clock::now();
    uint64_t total_edges = 0;

    for (size_t i = 0; i < docs.size(); ++i) {
        ssrag::ChunkMeta meta;
        meta.source_path = "STRING:" + docs[i].string_id;

        auto result = engine.ingest(store, std::move(docs[i].text), std::move(meta));
        total_edges += result.edges_created;

        if ((i + 1) % 100 == 0 || i + 1 == docs.size()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t_start).count();
            double rate = static_cast<double>(i + 1) / (static_cast<double>(elapsed) / 1000.0);
            std::cerr << "\r  [" << (i + 1) << "/" << docs.size() << "] "
                      << "dim=" << engine.effective_dim()
                      << " edges=" << total_edges
                      << " contrast=" << engine.estimated_contrast() << ":1"
                      << " (" << static_cast<int>(rate) << " docs/sec)"
                      << std::flush;
        }

        // Checkpoint every 1000 docs.
        if ((i + 1) % 1000 == 0) {
            store.save(store_path);
            engine.save_index(index_path);
        }
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - t_start).count();

    std::cerr << "\n\nIngestion complete.\n"
              << "  Documents: " << store.size() << "\n"
              << "  Effective dimensionality: " << engine.effective_dim() << "\n"
              << "  Estimated contrast: " << engine.estimated_contrast() << ":1\n"
              << "  Fold edges: " << total_edges << "\n"
              << "  Time: " << elapsed << "s\n";

    // Final save.
    std::cerr << "Saving index...\n";
    store.save(store_path);
    engine.save_index(index_path);
    std::cerr << "Done.\n";

    return 0;
}
