//
//  hash_index_test.cpp
//  blocksci hash index test - test all address types
//

#include <blocksci/blocksci.hpp>
#include <blocksci/address/address.hpp>
#include <blocksci/scripts/pubkey_script.hpp>
#include <blocksci/scripts/scripthash_script.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>
#include <blocksci/chain/access.hpp>
#include <internal/data_access.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace blocksci;

int main(int argc, const char * argv[]) {

    const std::string configPath = "/home/kloint/blocksci_data/config.json";

    Blockchain chain(configPath);

    std::cout << "=== Chain Info ===" << std::endl;
    std::cout << "Block count: " << chain.size() << std::endl;

    // Store test addresses by type
    std::map<std::string, std::vector<std::pair<std::string, uint32_t>>> testData;

    std::cout << "\n=== Finding Addresses by Type ===" << std::endl;

    int p2pkh_count = 0, p2sh_count = 0, p2wpkh_count = 0, p2wsh_count = 0, taproot_count = 0;
    const int MAX_PER_TYPE = 3;

    // Scan blocks to find addresses
    for (auto block : chain) {
        if (p2pkh_count >= MAX_PER_TYPE && p2sh_count >= MAX_PER_TYPE &&
            p2wpkh_count >= MAX_PER_TYPE && p2wsh_count >= MAX_PER_TYPE &&
            taproot_count >= MAX_PER_TYPE) {
            break;
        }

        RANGES_FOR(auto tx, block) {
            RANGES_FOR(auto output, tx.outputs()) {
                auto addr = output.getAddress();

                // P2PKH (1...)
                if (addr.type == AddressType::PUBKEYHASH && p2pkh_count < MAX_PER_TYPE) {
                    ScriptAddress<AddressType::PUBKEYHASH> script(addr.scriptNum, chain.getAccess());
                    auto addrStr = script.addressString();
                    testData["P2PKH"].push_back({addrStr, addr.scriptNum});
                    std::cout << "P2PKH: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                    p2pkh_count++;
                }
                // P2SH (3...)
                else if (addr.type == AddressType::SCRIPTHASH && p2sh_count < MAX_PER_TYPE) {
                    ScriptAddress<AddressType::SCRIPTHASH> script(addr.scriptNum, chain.getAccess());
                    auto addrStr = script.addressString();
                    testData["P2SH"].push_back({addrStr, addr.scriptNum});
                    std::cout << "P2SH: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                    p2sh_count++;
                }
                // P2WPKH (bc1q... 20 byte)
                else if (addr.type == AddressType::WITNESS_PUBKEYHASH && p2wpkh_count < MAX_PER_TYPE) {
                    ScriptAddress<AddressType::WITNESS_PUBKEYHASH> script(addr.scriptNum, chain.getAccess());
                    auto addrStr = script.addressString();
                    testData["P2WPKH"].push_back({addrStr, addr.scriptNum});
                    std::cout << "P2WPKH: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                    p2wpkh_count++;
                }
                // P2WSH (bc1q... 32 byte)
                else if (addr.type == AddressType::WITNESS_SCRIPTHASH && p2wsh_count < MAX_PER_TYPE) {
                    ScriptAddress<AddressType::WITNESS_SCRIPTHASH> script(addr.scriptNum, chain.getAccess());
                    auto addrStr = script.addressString();
                    testData["P2WSH"].push_back({addrStr, addr.scriptNum});
                    std::cout << "P2WSH: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                    p2wsh_count++;
                }
                // Taproot (bc1p...)
                else if (addr.type == AddressType::WITNESS_UNKNOWN && taproot_count < MAX_PER_TYPE) {
                    ScriptAddress<AddressType::WITNESS_UNKNOWN> script(addr.scriptNum, chain.getAccess());
                    // Only test Taproot (version 1, 32 bytes)
                    if (script.witnessVersion() == 1) {
                        auto addrStr = script.addressString();
                        if (!addrStr.empty()) {
                            testData["Taproot"].push_back({addrStr, addr.scriptNum});
                            std::cout << "Taproot: " << addrStr << " (scriptNum=" << addr.scriptNum << ")" << std::endl;
                            taproot_count++;
                        }
                    }
                }
            }
        }
    }

    std::cout << "\n=== Hash Index Lookup Test ===" << std::endl;

    int total = 0, found = 0;

    for (const auto &[typeName, addresses] : testData) {
        std::cout << "\n--- " << typeName << " ---" << std::endl;

        for (const auto &[addrStr, expectedScriptNum] : addresses) {
            total++;
            std::cout << "Lookup: " << addrStr << std::endl;

            auto result = getAddressFromString(addrStr, chain.getAccess());
            if (result) {
                if (result->scriptNum == expectedScriptNum) {
                    std::cout << "  OK! scriptNum=" << result->scriptNum << std::endl;
                    found++;
                } else {
                    std::cout << "  MISMATCH! got=" << result->scriptNum
                              << ", expected=" << expectedScriptNum << std::endl;
                }
            } else {
                std::cout << "  NOT FOUND in hash index (expected scriptNum="
                          << expectedScriptNum << ")" << std::endl;
            }
        }
    }

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Address Found: " << found << "/" << total << std::endl;

    // TX Hash Lookup Test
    std::cout << "\n=== TX Hash Lookup Test ===" << std::endl;

    int tx_total = 0, tx_found = 0;
    std::vector<uint32_t> testTxNums = {0, 1, 100, 1000, 10000, 100000};

    for (uint32_t txNum : testTxNums) {
        if (txNum >= txCount(chain)) continue;

        tx_total++;
        Transaction tx(txNum, chain.getAccess());
        auto txHash = tx.getHash();

        std::cout << "TX " << txNum << " hash: " << txHash.GetHex() << std::endl;

        try {
            Transaction result(txHash.GetHex(), chain.getAccess());
            if (result.txNum == txNum) {
                std::cout << "  OK! txNum=" << result.txNum << std::endl;
                tx_found++;
            } else {
                std::cout << "  MISMATCH! got=" << result.txNum << ", expected=" << txNum << std::endl;
            }
        } catch (const std::exception &e) {
            std::cout << "  NOT FOUND in hash index (" << e.what() << ")" << std::endl;
        }
    }

    std::cout << "\n=== Final Summary ===" << std::endl;
    std::cout << "Address: " << found << "/" << total << std::endl;
    std::cout << "TX Hash: " << tx_found << "/" << tx_total << std::endl;

    return 0;
}
