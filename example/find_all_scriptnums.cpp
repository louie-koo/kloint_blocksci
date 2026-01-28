//
//  find_all_scriptnums.cpp
//  Find all scriptNums for a given taproot address
//

#include <blocksci/blocksci.hpp>
#include <blocksci/chain/transaction.hpp>
#include <blocksci/chain/block.hpp>
#include <blocksci/chain/access.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>

#include <internal/data_access.hpp>
#include <internal/script_access.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <set>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";
    std::string targetAddr = "bc1pmanxyggsck006vg8wayvlltyffvwwpfpcpv8qsqj0tg5fml7hmmsc7zfsj";

    if (argc > 1) {
        targetAddr = argv[1];
    }

    std::cout << "=== Find All ScriptNums for Taproot Address ===" << std::endl;
    std::cout << "Target: " << targetAddr << std::endl;

    Blockchain chain(configPath);
    auto &access = chain.getAccess();
    auto &scripts = access.getScripts();

    // Get total witness_unknown script count
    uint32_t totalScripts = scripts.scriptCount(DedupAddressType::WITNESS_UNKNOWN);
    std::cout << "Total WITNESS_UNKNOWN scripts: " << totalScripts << std::endl;

    // Find all scriptNums with matching address
    std::vector<uint32_t> matchingScriptNums;

    std::cout << "\nScanning all WITNESS_UNKNOWN scripts..." << std::endl;

    auto progressInterval = totalScripts / 100;
    if (progressInterval == 0) progressInterval = 1;

    for (uint32_t scriptNum = 1; scriptNum <= totalScripts; scriptNum++) {
        if (scriptNum % progressInterval == 0) {
            std::cout << "\r  Progress: " << (scriptNum * 100 / totalScripts) << "%" << std::flush;
        }

        try {
            ScriptAddress<AddressType::WITNESS_UNKNOWN> script(scriptNum, access);
            std::string addrStr = script.addressString();

            if (addrStr == targetAddr) {
                matchingScriptNums.push_back(scriptNum);
            }
        } catch (...) {
            // Skip invalid scripts
        }
    }
    std::cout << "\r  Progress: 100%" << std::endl;

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Found " << matchingScriptNums.size() << " scriptNums for address: " << targetAddr << std::endl;

    if (matchingScriptNums.empty()) {
        std::cout << "No matching scriptNums found!" << std::endl;
        return 1;
    }

    // For each scriptNum, get transactions
    std::set<uint32_t> allTxNums;
    int totalTxCount = 0;

    for (uint32_t scriptNum : matchingScriptNums) {
        Address addr(scriptNum, AddressType::WITNESS_UNKNOWN, access);
        auto txes = addr.getTransactions();

        int txCount = 0;
        for (auto tx : txes) {
            allTxNums.insert(tx.txNum);
            txCount++;
        }

        std::cout << "\nScriptNum " << scriptNum << ": " << txCount << " TX(s)" << std::endl;
        totalTxCount += txCount;
    }

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Total scriptNums: " << matchingScriptNums.size() << std::endl;
    std::cout << "Total unique TXs: " << allTxNums.size() << std::endl;
    std::cout << "Total TX references: " << totalTxCount << std::endl;

    // Show all transactions sorted by txNum
    std::cout << "\n=== All Transactions ===" << std::endl;
    int count = 0;
    for (uint32_t txNum : allTxNums) {
        Transaction tx(txNum, access);
        std::cout << count + 1 << ". " << tx.getHash().GetHex()
                  << " (block " << tx.block().height() << ")" << std::endl;
        count++;
        if (count >= 30) {
            std::cout << "... (showing first 30)" << std::endl;
            break;
        }
    }

    return 0;
}
