//
//  hash_index_create_test.cpp
//  Test hash index creation and lookup for specific addresses
//

#include <blocksci/blocksci.hpp>
#include <blocksci/address/address.hpp>
#include <blocksci/scripts/pubkey_script.hpp>
#include <blocksci/scripts/scripthash_script.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>
#include <blocksci/core/script_data.hpp>

#include <internal/hash_index.hpp>
#include <internal/script_access.hpp>
#include <internal/data_access.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace blocksci;

int main(int argc, const char * argv[]) {

    const std::string configPath = "/home/kloint/blocksci_data/config.json";

    // Store test addresses by type
    std::map<std::string, std::vector<std::tuple<std::string, uint32_t, AddressType::Enum>>> testData;

    std::cout << "=== Phase 1: Index Creation ===" << std::endl;

    // Phase 1: Open blockchain and add addresses to hash index
    {
        Blockchain chain(configPath);
        auto &access = chain.getAccess();

        std::cout << "Block count: " << chain.size() << std::endl;

        // Open hash index in write mode
        HashIndex hashIndex(access.config.hashIndexFilePath(), false);

        std::cout << "\nFinding and Indexing Addresses..." << std::endl;

        int p2pkh_count = 0, p2wpkh_count = 0, taproot_count = 0;
        const int MAX_PER_TYPE = 3;

        // Scan blocks to find addresses and add to hash index
        for (auto block : chain) {
            if (p2pkh_count >= MAX_PER_TYPE && p2wpkh_count >= MAX_PER_TYPE && taproot_count >= MAX_PER_TYPE) {
                break;
            }

            RANGES_FOR(auto tx, block) {
                RANGES_FOR(auto output, tx.outputs()) {
                    auto addr = output.getAddress();

                    // P2PKH (1...)
                    if (addr.type == AddressType::PUBKEYHASH && p2pkh_count < MAX_PER_TYPE) {
                        ScriptAddress<AddressType::PUBKEYHASH> script(addr.scriptNum, access);
                        auto addrStr = script.addressString();
                        auto pubkeyHash = script.getPubkeyHash();

                        // Add to hash index
                        std::vector<std::pair<uint160, uint32_t>> rows;
                        rows.emplace_back(pubkeyHash, addr.scriptNum);
                        hashIndex.addAddresses<AddressType::PUBKEYHASH>(std::move(rows));

                        testData["P2PKH"].push_back({addrStr, addr.scriptNum, addr.type});
                        std::cout << "Indexed P2PKH: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                        p2pkh_count++;
                    }
                    // P2WPKH (bc1q... 20 byte)
                    else if (addr.type == AddressType::WITNESS_PUBKEYHASH && p2wpkh_count < MAX_PER_TYPE) {
                        ScriptAddress<AddressType::WITNESS_PUBKEYHASH> script(addr.scriptNum, access);
                        auto addrStr = script.addressString();
                        auto pubkeyHash = script.getPubkeyHash();

                        // Add to hash index
                        std::vector<std::pair<uint160, uint32_t>> rows;
                        rows.emplace_back(pubkeyHash, addr.scriptNum);
                        hashIndex.addAddresses<AddressType::WITNESS_PUBKEYHASH>(std::move(rows));

                        testData["P2WPKH"].push_back({addrStr, addr.scriptNum, addr.type});
                        std::cout << "Indexed P2WPKH: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                        p2wpkh_count++;
                    }
                    // Taproot (bc1p...)
                    else if (addr.type == AddressType::WITNESS_UNKNOWN && taproot_count < MAX_PER_TYPE) {
                        // Get script data directly (returns tuple)
                        auto scriptDataTuple = access.getScripts().getScriptData<DedupAddressType::WITNESS_UNKNOWN>(addr.scriptNum);
                        auto scriptData = std::get<0>(scriptDataTuple);
                        if (scriptData->witnessVersion == 1 && scriptData->scriptData.size() == 32) {
                            ScriptAddress<AddressType::WITNESS_UNKNOWN> script(addr.scriptNum, access);
                            auto addrStr = script.addressString();
                            if (!addrStr.empty()) {
                                // Create uint256 from witness program
                                uint256 witnessProgram;
                                memcpy(witnessProgram.begin(), scriptData->scriptData.begin(), 32);

                                // Add to hash index
                                std::vector<std::pair<uint256, uint32_t>> rows;
                                rows.emplace_back(witnessProgram, addr.scriptNum);
                                hashIndex.addAddresses<AddressType::WITNESS_UNKNOWN>(std::move(rows));

                                testData["Taproot"].push_back({addrStr, addr.scriptNum, addr.type});
                                std::cout << "Indexed Taproot: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                                taproot_count++;
                            }
                        }
                    }
                }
            }
        }

        std::cout << "\nClosing hash index (flushing to disk)..." << std::endl;
        // hashIndex destructor will flush data to disk
    }

    std::cout << "\n=== Phase 2: Lookup Test ===" << std::endl;

    // Phase 2: Reopen blockchain with fresh HashIndex and test lookups
    {
        Blockchain chain(configPath);
        auto &access = chain.getAccess();

        int total = 0, found = 0;

        for (const auto &[typeName, addresses] : testData) {
            std::cout << "\n--- " << typeName << " ---" << std::endl;

            for (const auto &[addrStr, expectedScriptNum, expectedType] : addresses) {
                total++;
                std::cout << "Lookup: " << addrStr << std::endl;

                auto result = getAddressFromString(addrStr, access);
                if (result) {
                    if (result->scriptNum == expectedScriptNum && result->type == expectedType) {
                        std::cout << "  OK! scriptNum=" << result->scriptNum << std::endl;
                        found++;
                    } else {
                        std::cout << "  MISMATCH! got scriptNum=" << result->scriptNum
                                  << ", type=" << static_cast<int>(result->type)
                                  << ", expected scriptNum=" << expectedScriptNum
                                  << ", type=" << static_cast<int>(expectedType) << std::endl;
                    }
                } else {
                    std::cout << "  NOT FOUND in hash index!" << std::endl;
                }
            }
        }

        std::cout << "\n=== Summary ===" << std::endl;
        std::cout << "Indexed and Found: " << found << "/" << total << std::endl;

        if (found == total) {
            std::cout << "\nSUCCESS! All address types are working correctly." << std::endl;
            std::cout << "You can now run the full hash-index-update:" << std::endl;
            std::cout << "  ./tools/parser/blocksci_parser " << configPath << " hash-index-update" << std::endl;
        } else {
            std::cout << "\nSome lookups failed. Check the code." << std::endl;
        }
    }

    return 0;
}
