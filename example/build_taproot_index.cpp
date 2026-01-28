//
//  build_taproot_index.cpp
//  Build reverse index: witness_program -> [scriptNums]
//  Save to file for fast lookup
//

#include <blocksci/blocksci.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>

#include <internal/data_access.hpp>
#include <internal/script_access.hpp>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";
    std::string outputPath = "/home/kloint/blocksci_data/taproot_index.bin";

    std::cout << "=== Build Taproot Reverse Index ===" << std::endl;

    Blockchain chain(configPath);
    auto &access = chain.getAccess();
    auto &scripts = access.getScripts();

    uint32_t totalScripts = scripts.scriptCount(DedupAddressType::WITNESS_UNKNOWN);
    std::cout << "Total WITNESS_UNKNOWN scripts: " << totalScripts << std::endl;

    // Map: address_string -> vector of scriptNums
    std::unordered_map<std::string, std::vector<uint32_t>> index;

    std::cout << "Building index..." << std::endl;

    auto progressInterval = totalScripts / 100;
    if (progressInterval == 0) progressInterval = 1;

    uint32_t taprootCount = 0;

    for (uint32_t scriptNum = 1; scriptNum <= totalScripts; scriptNum++) {
        if (scriptNum % progressInterval == 0) {
            std::cout << "\r  Progress: " << (scriptNum * 100 / totalScripts) << "% (taproot: " << taprootCount << ")" << std::flush;
        }

        try {
            ScriptAddress<AddressType::WITNESS_UNKNOWN> script(scriptNum, access);

            // Only index Taproot (version 1, 32 bytes)
            if (script.witnessVersion() == 1) {
                std::string addrStr = script.addressString();
                if (!addrStr.empty() && addrStr.substr(0, 4) == "bc1p") {
                    index[addrStr].push_back(scriptNum);
                    taprootCount++;
                }
            }
        } catch (...) {
            // Skip invalid scripts
        }
    }
    std::cout << "\r  Progress: 100%" << std::endl;

    std::cout << "\n=== Statistics ===" << std::endl;
    std::cout << "Total Taproot scripts: " << taprootCount << std::endl;
    std::cout << "Unique Taproot addresses: " << index.size() << std::endl;

    // Count addresses with multiple scriptNums
    uint32_t multiCount = 0;
    uint32_t maxScriptNums = 0;
    for (const auto &pair : index) {
        if (pair.second.size() > 1) {
            multiCount++;
            if (pair.second.size() > maxScriptNums) {
                maxScriptNums = pair.second.size();
            }
        }
    }
    std::cout << "Addresses with multiple scriptNums: " << multiCount << std::endl;
    std::cout << "Max scriptNums per address: " << maxScriptNums << std::endl;

    // Save to binary file
    std::cout << "\nSaving index to: " << outputPath << std::endl;

    std::ofstream out(outputPath, std::ios::binary);
    uint32_t indexSize = index.size();
    out.write(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));

    for (const auto &pair : index) {
        uint32_t addrLen = pair.first.size();
        out.write(reinterpret_cast<char*>(&addrLen), sizeof(addrLen));
        out.write(pair.first.data(), addrLen);

        uint32_t numScripts = pair.second.size();
        out.write(reinterpret_cast<char*>(&numScripts), sizeof(numScripts));
        out.write(reinterpret_cast<const char*>(pair.second.data()), numScripts * sizeof(uint32_t));
    }
    out.close();

    std::cout << "Done! Index saved." << std::endl;

    return 0;
}
