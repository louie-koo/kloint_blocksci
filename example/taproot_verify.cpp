//
//  taproot_verify.cpp
//  Pick random Taproot addresses and verify TX count
//

#include <blocksci/blocksci.hpp>
#include <blocksci/chain/transaction.hpp>
#include <blocksci/chain/block.hpp>
#include <blocksci/chain/access.hpp>
#include <blocksci/address/address.hpp>
#include <blocksci/script.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>

#include <internal/data_access.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <set>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";

    std::cout << "=== Taproot Address Verification ===" << std::endl;

    Blockchain chain(configPath);
    auto &access = chain.getAccess();

    std::cout << "Chain loaded: " << chain.size() << " blocks" << std::endl;

    // Collect Taproot addresses from recent blocks
    std::vector<std::pair<Address, std::string>> taprootAddresses;

    int startBlock = std::max(0, static_cast<int>(chain.size()) - 1000);
    std::cout << "Scanning blocks " << startBlock << " to " << chain.size() - 1 << " for Taproot addresses..." << std::endl;

    std::set<uint32_t> seenScriptNums;

    for (int i = startBlock; i < static_cast<int>(chain.size()) && taprootAddresses.size() < 100; i++) {
        Block block = chain[i];
        for (auto tx : block) {
            for (auto output : tx.outputs()) {
                Address outAddr = output.getAddress();
                if (outAddr.type == AddressType::WITNESS_UNKNOWN) {
                    if (seenScriptNums.find(outAddr.scriptNum) == seenScriptNums.end()) {
                        seenScriptNums.insert(outAddr.scriptNum);

                        // Get the actual bech32m address string
                        ScriptAddress<AddressType::WITNESS_UNKNOWN> script(outAddr.scriptNum, access);
                        std::string addrStr = script.addressString();

                        if (!addrStr.empty()) {
                            taprootAddresses.push_back({outAddr, addrStr});
                        }
                    }
                }
            }
            if (taprootAddresses.size() >= 100) break;
        }
    }

    std::cout << "Found " << taprootAddresses.size() << " unique Taproot addresses" << std::endl;

    // Randomly select 10
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(taprootAddresses.begin(), taprootAddresses.end(), gen);

    int testCount = std::min(10, static_cast<int>(taprootAddresses.size()));

    std::cout << "\n=== Testing " << testCount << " random Taproot addresses ===" << std::endl;
    std::cout << "\nFormat: Address | BlockSci TX Count | Script Num\n" << std::endl;

    for (int i = 0; i < testCount; i++) {
        Address addr = taprootAddresses[i].first;
        std::string addrStr = taprootAddresses[i].second;

        // Count transactions in BlockSci
        auto txes = addr.getTransactions();
        int txCount = 0;
        for (auto tx : txes) {
            txCount++;
        }

        std::cout << i + 1 << ". " << addrStr << std::endl;
        std::cout << "   BlockSci TX count: " << txCount << std::endl;
        std::cout << "   Script num: " << addr.scriptNum << std::endl;

        // Show first few TX hashes for verification
        if (txCount > 0) {
            std::cout << "   Sample TXs:" << std::endl;
            int shown = 0;
            for (auto tx : txes) {
                std::cout << "     - " << tx.getHash().GetHex() << " (block " << tx.block().height() << ")" << std::endl;
                shown++;
                if (shown >= 3) break;
            }
        }
        std::cout << std::endl;
    }

    std::cout << "\n=== Verification URLs ===" << std::endl;
    std::cout << "Use these URLs to verify TX counts on blockstream.info:\n" << std::endl;

    for (int i = 0; i < testCount; i++) {
        std::string addrStr = taprootAddresses[i].second;
        std::cout << i + 1 << ". https://blockstream.info/address/" << addrStr << std::endl;
    }

    return 0;
}
