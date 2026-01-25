//
//  hash_index_debug.cpp
//  Debug hash index content
//

#include <blocksci/blocksci.hpp>
#include <blocksci/scripts/pubkey_script.hpp>
#include <internal/hash_index.hpp>
#include <internal/data_access.hpp>
#include <internal/chain_access.hpp>
#include <internal/script_access.hpp>

#include <iostream>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    const std::string configPath = "/home/kloint/blocksci_data/config.json";

    Blockchain chain(configPath);
    auto &access = chain.getAccess();

    std::cout << "=== Hash Index Debug ===" << std::endl;
    std::cout << "Block count: " << chain.size() << std::endl;

    // Count entries in each column family
    std::cout << "\n=== Column Family Counts ===" << std::endl;

    auto &hashIndex = access.getHashIndex();

    std::cout << "PUBKEYHASH: " << hashIndex.countColumn(AddressType::PUBKEYHASH) << std::endl;
    std::cout << "SCRIPTHASH: " << hashIndex.countColumn(AddressType::SCRIPTHASH) << std::endl;
    std::cout << "WITNESS_PUBKEYHASH: " << hashIndex.countColumn(AddressType::WITNESS_PUBKEYHASH) << std::endl;
    std::cout << "WITNESS_SCRIPTHASH: " << hashIndex.countColumn(AddressType::WITNESS_SCRIPTHASH) << std::endl;
    std::cout << "WITNESS_UNKNOWN: " << hashIndex.countColumn(AddressType::WITNESS_UNKNOWN) << std::endl;
    std::cout << "TX count: " << hashIndex.countTxes() << std::endl;

    // Test specific lookup for scriptNum=745
    std::cout << "\n=== Test Lookup ===" << std::endl;

    ScriptAddress<AddressType::PUBKEYHASH> script745(745, access);
    auto addrStr745 = script745.addressString();
    auto pubkeyHash745 = script745.getPubkeyHash();

    std::cout << "scriptNum=745 address: " << addrStr745 << std::endl;
    std::cout << "pubkeyHash: ";
    for (int i = 0; i < 20; i++) {
        printf("%02x", pubkeyHash745.begin()[i]);
    }
    std::cout << std::endl;

    // Lookup in hash index
    auto result = hashIndex.getPubkeyHashIndex(pubkeyHash745);
    if (result) {
        std::cout << "Found in hash index! scriptNum=" << *result << std::endl;
    } else {
        std::cout << "NOT found in hash index!" << std::endl;
    }

    // Also try via getAddressFromString
    std::cout << "\ngetAddressFromString lookup:" << std::endl;
    auto addrResult = getAddressFromString(addrStr745, access);
    if (addrResult) {
        std::cout << "Found! scriptNum=" << addrResult->scriptNum << ", type=" << static_cast<int>(addrResult->type) << std::endl;
    } else {
        std::cout << "NOT found!" << std::endl;
    }

    // Find when scriptNum=745 first appears as an output
    std::cout << "\n=== Find First Output for scriptNum=745 ===" << std::endl;
    uint32_t foundTxNum = 0;
    bool found = false;
    for (auto block : chain) {
        RANGES_FOR(auto tx, block) {
            RANGES_FOR(auto output, tx.outputs()) {
                auto addr = output.getAddress();
                if (addr.type == AddressType::PUBKEYHASH && addr.scriptNum == 745) {
                    foundTxNum = tx.txNum;
                    found = true;
                    std::cout << "scriptNum=745 first output in txNum=" << foundTxNum
                              << " (block " << block.height() << ")" << std::endl;
                    break;
                }
            }
            if (found) break;
        }
        if (found) break;
    }

    if (!found) {
        std::cout << "scriptNum=745 not found as output" << std::endl;
    }

    // Simulate what processTx does for txNum=746
    std::cout << "\n=== Simulate processTx for txNum=746 ===" << std::endl;

    // Get raw access like the parser does
    ChainAccess chainAccess{access.config.chainDirectory(), 0, false};
    ScriptAccess scriptAccess{access.config.scriptsDirectory()};

    auto rawTx = chainAccess.getTx(746);
    std::cout << "Raw tx has " << rawTx->outputCount << " outputs" << std::endl;

    auto outputs = ranges::make_subrange(rawTx->beginOutputs(), rawTx->endOutputs());
    for (auto &txout : outputs) {
        auto addressNum = txout.getAddressNum();
        auto addressType = txout.getType();
        std::cout << "Raw output: type=" << static_cast<int>(addressType)
                  << " addressNum=" << addressNum << std::endl;

        if (addressType == AddressType::PUBKEYHASH) {
            auto script = scriptAccess.getScriptData<DedupAddressType::PUBKEY>(addressNum);
            std::cout << "  Script data address: ";
            for (int i = 0; i < 20; i++) {
                printf("%02x", script->address.begin()[i]);
            }
            std::cout << std::endl;
        }
    }

    return 0;
}
