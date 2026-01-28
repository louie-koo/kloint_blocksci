//
//  taproot_lookup_fast.cpp
//  Fast taproot lookup using RocksDB (instant loading)
//

#include <blocksci/blocksci.hpp>
#include <blocksci/chain/transaction.hpp>
#include <blocksci/address/address.hpp>

#include <internal/data_access.hpp>

#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";
    std::string dbPath = "/home/kloint/blocksci_data/taproot_rocksdb";
    std::string targetAddr = "bc1pmanxyggsck006vg8wayvlltyffvwwpfpcpv8qsqj0tg5fml7hmmsc7zfsj";

    if (argc > 1) {
        targetAddr = argv[1];
    }

    std::cout << "=== Taproot Fast Lookup (RocksDB) ===" << std::endl;
    std::cout << "Target: " << targetAddr << std::endl;

    // Open RocksDB
    auto startOpen = std::chrono::high_resolution_clock::now();

    rocksdb::Options options;
    options.IncreaseParallelism();
    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, dbPath, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
        return 1;
    }

    auto endOpen = std::chrono::high_resolution_clock::now();
    auto openTime = std::chrono::duration_cast<std::chrono::milliseconds>(endOpen - startOpen).count();
    std::cout << "DB open time: " << openTime << "ms" << std::endl;

    // Lookup
    auto startLookup = std::chrono::high_resolution_clock::now();

    std::string value;
    status = db->Get(rocksdb::ReadOptions(), targetAddr, &value);

    auto endLookup = std::chrono::high_resolution_clock::now();
    auto lookupTime = std::chrono::duration_cast<std::chrono::microseconds>(endLookup - startLookup).count();

    if (!status.ok()) {
        std::cout << "\nAddress not found!" << std::endl;
        delete db;
        return 1;
    }

    // Parse value
    uint32_t numScripts;
    memcpy(&numScripts, value.data(), sizeof(numScripts));

    std::vector<uint32_t> scriptNums(numScripts);
    memcpy(scriptNums.data(), value.data() + sizeof(numScripts), numScripts * sizeof(uint32_t));

    std::cout << "Lookup time: " << lookupTime << " microseconds" << std::endl;
    std::cout << "Found " << numScripts << " scriptNums" << std::endl;

    delete db;

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
