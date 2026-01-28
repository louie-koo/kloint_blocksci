//
//  build_taproot_rocksdb.cpp
//  Build taproot index using RocksDB for instant loading
//

#include <blocksci/blocksci.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>

#include <internal/data_access.hpp>
#include <internal/script_access.hpp>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";
    std::string dbPath = "/home/kloint/blocksci_data/taproot_rocksdb";

    std::cout << "=== Build Taproot RocksDB Index ===" << std::endl;

    Blockchain chain(configPath);
    auto &access = chain.getAccess();
    auto &scripts = access.getScripts();

    uint32_t totalScripts = scripts.scriptCount(DedupAddressType::WITNESS_UNKNOWN);
    std::cout << "Total WITNESS_UNKNOWN scripts: " << totalScripts << std::endl;

    // Collect all scriptNums per address
    std::unordered_map<std::string, std::vector<uint32_t>> index;

    std::cout << "Scanning scripts..." << std::endl;

    auto progressInterval = totalScripts / 100;
    if (progressInterval == 0) progressInterval = 1;

    uint32_t taprootCount = 0;

    for (uint32_t scriptNum = 1; scriptNum <= totalScripts; scriptNum++) {
        if (scriptNum % progressInterval == 0) {
            std::cout << "\r  Progress: " << (scriptNum * 100 / totalScripts) << "%" << std::flush;
        }

        try {
            ScriptAddress<AddressType::WITNESS_UNKNOWN> script(scriptNum, access);
            if (script.witnessVersion() == 1) {
                std::string addrStr = script.addressString();
                if (!addrStr.empty() && addrStr.substr(0, 4) == "bc1p") {
                    index[addrStr].push_back(scriptNum);
                    taprootCount++;
                }
            }
        } catch (...) {}
    }
    std::cout << "\r  Progress: 100%" << std::endl;

    std::cout << "\nTotal Taproot scripts: " << taprootCount << std::endl;
    std::cout << "Unique addresses: " << index.size() << std::endl;

    // Create RocksDB
    std::cout << "\nCreating RocksDB at: " << dbPath << std::endl;

    rocksdb::Options options;
    options.create_if_missing = true;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();

    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::Open(options, dbPath, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
        return 1;
    }

    // Write data
    std::cout << "Writing to RocksDB..." << std::endl;
    rocksdb::WriteBatch batch;
    uint32_t count = 0;

    for (const auto &pair : index) {
        const std::string &addr = pair.first;
        const std::vector<uint32_t> &scriptNums = pair.second;

        // Value format: count (4 bytes) + scriptNums (4 bytes each)
        std::string value;
        uint32_t numScripts = scriptNums.size();
        value.append(reinterpret_cast<const char*>(&numScripts), sizeof(numScripts));
        value.append(reinterpret_cast<const char*>(scriptNums.data()), scriptNums.size() * sizeof(uint32_t));

        batch.Put(addr, value);
        count++;

        if (count % 1000000 == 0) {
            db->Write(rocksdb::WriteOptions(), &batch);
            batch.Clear();
            std::cout << "\r  Written: " << count << " / " << index.size() << std::flush;
        }
    }

    // Write remaining
    db->Write(rocksdb::WriteOptions(), &batch);
    std::cout << "\r  Written: " << count << " / " << index.size() << std::endl;

    // Compact
    std::cout << "Compacting..." << std::endl;
    db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);

    delete db;
    std::cout << "Done!" << std::endl;

    return 0;
}
