//
//  taproot_lookup.cpp
//  Fast lookup of all scriptNums for a taproot address using pre-built index
//

#include <blocksci/blocksci.hpp>
#include <blocksci/chain/transaction.hpp>
#include <blocksci/address/address.hpp>

#include <internal/data_access.hpp>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <set>
#include <chrono>

using namespace blocksci;

// Load index from file
std::unordered_map<std::string, std::vector<uint32_t>> loadIndex(const std::string &path) {
    std::unordered_map<std::string, std::vector<uint32_t>> index;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open index file: " + path);
    }

    uint32_t indexSize;
    in.read(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));

    for (uint32_t i = 0; i < indexSize; i++) {
        uint32_t addrLen;
        in.read(reinterpret_cast<char*>(&addrLen), sizeof(addrLen));

        std::string addr(addrLen, '\0');
        in.read(&addr[0], addrLen);

        uint32_t numScripts;
        in.read(reinterpret_cast<char*>(&numScripts), sizeof(numScripts));

        std::vector<uint32_t> scriptNums(numScripts);
        in.read(reinterpret_cast<char*>(scriptNums.data()), numScripts * sizeof(uint32_t));

        index[addr] = std::move(scriptNums);
    }

    return index;
}

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";
    std::string indexPath = "/home/kloint/blocksci_data/taproot_index.bin";
    std::string targetAddr = "bc1pmanxyggsck006vg8wayvlltyffvwwpfpcpv8qsqj0tg5fml7hmmsc7zfsj";

    if (argc > 1) {
        targetAddr = argv[1];
    }

    std::cout << "=== Taproot Address Lookup ===" << std::endl;
    std::cout << "Target: " << targetAddr << std::endl;

    // Load index
    auto startLoad = std::chrono::high_resolution_clock::now();
    std::cout << "\nLoading index..." << std::flush;
    auto index = loadIndex(indexPath);
    auto endLoad = std::chrono::high_resolution_clock::now();
    auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endLoad - startLoad).count();
    std::cout << " done (" << loadTime << "ms, " << index.size() << " addresses)" << std::endl;

    // Lookup
    auto startLookup = std::chrono::high_resolution_clock::now();
    auto it = index.find(targetAddr);
    auto endLookup = std::chrono::high_resolution_clock::now();
    auto lookupTime = std::chrono::duration_cast<std::chrono::microseconds>(endLookup - startLookup).count();

    if (it == index.end()) {
        std::cout << "\nAddress not found in index!" << std::endl;
        return 1;
    }

    const auto &scriptNums = it->second;
    std::cout << "\nLookup time: " << lookupTime << " microseconds" << std::endl;
    std::cout << "Found " << scriptNums.size() << " scriptNums" << std::endl;

    // Load blockchain for TX lookup
    Blockchain chain(configPath);
    auto &access = chain.getAccess();

    // Get all transactions
    std::set<uint32_t> allTxNums;

    for (uint32_t scriptNum : scriptNums) {
        Address addr(scriptNum, AddressType::WITNESS_UNKNOWN, access);
        for (auto tx : addr.getTransactions()) {
            allTxNums.insert(tx.txNum);
        }
    }

    std::cout << "Total unique TXs: " << allTxNums.size() << std::endl;

    // Show transactions
    std::cout << "\n=== Transactions ===" << std::endl;
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
