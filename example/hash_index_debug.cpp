//
//  hash_index_debug.cpp
//  Debug hash index content - Taproot verification
//

#include <blocksci/blocksci.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>
#include <internal/hash_index.hpp>
#include <internal/data_access.hpp>
#include <internal/script_access.hpp>

#include <iostream>
#include <vector>
#include <cstring>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    const std::string configPath = "/home/kloint/blocksci_data/config.json";

    Blockchain chain(configPath);
    auto &access = chain.getAccess();

    std::cout << "=== Taproot Verification (10 addresses) ===" << std::endl;

    auto &hashIndex = access.getHashIndex();
    ScriptAccess scriptAccess{access.config.scriptsDirectory()};

    // Find 10 standard Taproot addresses (version=1, size=32)
    std::vector<std::pair<uint32_t, std::string>> taprootAddresses;
    int found = 0;
    int checked = 0;
    const int TARGET = 10;

    // Scan WITNESS_UNKNOWN scripts to find standard Taproot (start from higher scriptNum to avoid early duplicates)
    for (uint32_t scriptNum = 100000; found < TARGET && scriptNum < 10000000; scriptNum++) {
        checked++;
        try {
            auto scriptTuple = scriptAccess.getScriptData<DedupAddressType::WITNESS_UNKNOWN>(scriptNum);
            auto scriptData = std::get<0>(scriptTuple);

            // Only standard Taproot: version=1, size=32
            if (scriptData->witnessVersion == 1 && scriptData->scriptData.size() == 32) {
                ScriptAddress<AddressType::WITNESS_UNKNOWN> script(scriptNum, access);
                auto addrStr = script.addressString();

                if (!addrStr.empty()) {
                    taprootAddresses.push_back({scriptNum, addrStr});
                    found++;
                }
            }
        } catch (...) {
            // Script doesn't exist, skip
            break;
        }
    }

    std::cout << "Checked " << checked << " scripts, found " << found << " standard Taproot addresses\n\n";

    // Verify each address
    int success = 0;
    int mismatch = 0;
    int notfound = 0;

    for (const auto &[scriptNum, addrStr] : taprootAddresses) {
        std::cout << "scriptNum=" << scriptNum << ": " << addrStr << std::endl;

        // Get witness program
        auto scriptTuple = scriptAccess.getScriptData<DedupAddressType::WITNESS_UNKNOWN>(scriptNum);
        auto scriptData = std::get<0>(scriptTuple);

        uint256 witnessProgram;
        memcpy(witnessProgram.begin(), scriptData->scriptData.begin(), 32);

        // Lookup in hash index
        auto result = hashIndex.getWitnessUnknownIndex(witnessProgram);
        if (result) {
            if (*result == scriptNum) {
                std::cout << "  -> OK!" << std::endl;
                success++;
            } else {
                std::cout << "  -> MISMATCH! got=" << *result << " (duplicate witness program)" << std::endl;
                mismatch++;
            }
        } else {
            std::cout << "  -> NOT FOUND!" << std::endl;
            notfound++;
        }
    }

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Success: " << success << "/" << found << std::endl;
    std::cout << "Mismatch (duplicates): " << mismatch << std::endl;
    std::cout << "Not Found: " << notfound << std::endl;

    return 0;
}
