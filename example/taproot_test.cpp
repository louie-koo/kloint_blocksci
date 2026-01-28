//
//  taproot_test.cpp
//  Test Taproot (bc1p) address transactions in BlockSci
//

#include <blocksci/blocksci.hpp>
#include <blocksci/chain/transaction.hpp>
#include <blocksci/chain/block.hpp>
#include <blocksci/chain/access.hpp>
#include <blocksci/address/address.hpp>

#include <internal/data_access.hpp>
#include <internal/hash_index.hpp>

#include <iostream>
#include <string>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";

    std::cout << "=== Taproot Address Test ===" << std::endl;

    Blockchain chain(configPath);
    auto &access = chain.getAccess();

    std::cout << "Chain loaded: " << chain.size() << " blocks" << std::endl;

    // Test Taproot addresses (bc1p...)
    std::vector<std::string> taprootAddresses = {
        "bc1p5d7rjq7g6rdk2yhzks9smlaqtedr4dekq08ge8ztwac72sfr9rusxg3297",
        "bc1pxwww0ct9ue7e8tdnlmug5m2tamfn7q06sahstg39ys4c9f3340qqxrdu9k",
    };

    for (const auto& addrStr : taprootAddresses) {
        std::cout << "\n--- Testing address: " << addrStr << " ---" << std::endl;

        auto addrOpt = getAddressFromString(addrStr, access);
        if (!addrOpt) {
            std::cout << "Address NOT FOUND in BlockSci data" << std::endl;
            continue;
        }

        Address addr = *addrOpt;
        std::cout << "Address type: " << static_cast<int>(addr.type) << " (";

        switch(addr.type) {
            case AddressType::PUBKEY: std::cout << "PUBKEY"; break;
            case AddressType::PUBKEYHASH: std::cout << "PUBKEYHASH"; break;
            case AddressType::SCRIPTHASH: std::cout << "SCRIPTHASH"; break;
            case AddressType::MULTISIG: std::cout << "MULTISIG"; break;
            case AddressType::WITNESS_PUBKEYHASH: std::cout << "WITNESS_PUBKEYHASH"; break;
            case AddressType::WITNESS_SCRIPTHASH: std::cout << "WITNESS_SCRIPTHASH"; break;
            case AddressType::WITNESS_UNKNOWN: std::cout << "WITNESS_UNKNOWN (Taproot)"; break;
            case AddressType::NONSTANDARD: std::cout << "NONSTANDARD"; break;
            case AddressType::NULL_DATA: std::cout << "NULL_DATA"; break;
            default: std::cout << "UNKNOWN"; break;
        }
        std::cout << ")" << std::endl;
        std::cout << "Script num: " << addr.scriptNum << std::endl;

        auto txes = addr.getTransactions();
        int txCount = 0;
        for (auto tx : txes) {
            std::cout << "\n  TX #" << txCount + 1 << ":" << std::endl;
            std::cout << "    Hash: " << tx.getHash().GetHex() << std::endl;
            std::cout << "    Block: " << tx.block().height() << std::endl;
            std::cout << "    TxNum: " << tx.txNum << std::endl;
            std::cout << "    Inputs: " << tx.inputCount() << ", Outputs: " << tx.outputCount() << std::endl;

            txCount++;
            if (txCount >= 5) {
                std::cout << "  ... (showing first 5 TXs only)" << std::endl;
                break;
            }
        }

        if (txCount == 0) {
            std::cout << "  No transactions found for this address" << std::endl;
        } else {
            std::cout << "\n  Total transactions shown: " << txCount << std::endl;
        }
    }

    // Scan for WITNESS_UNKNOWN addresses in recent blocks
    std::cout << "\n\n=== Scanning for Taproot outputs in recent blocks ===" << std::endl;

    int startBlock = std::max(0, static_cast<int>(chain.size()) - 100);
    int taprootOutputCount = 0;

    for (int i = startBlock; i < static_cast<int>(chain.size()) && taprootOutputCount < 10; i++) {
        Block block = chain[i];
        for (auto tx : block) {
            for (auto output : tx.outputs()) {
                Address outAddr = output.getAddress();
                if (outAddr.type == AddressType::WITNESS_UNKNOWN) {
                    std::cout << "\nBlock " << i << ", TX: " << tx.getHash().GetHex().substr(0, 16) << "..." << std::endl;
                    std::cout << "  Taproot output, value: " << output.getValue() << " satoshis" << std::endl;
                    std::cout << "  Script num: " << outAddr.scriptNum << std::endl;
                    taprootOutputCount++;
                    if (taprootOutputCount >= 10) break;
                }
            }
            if (taprootOutputCount >= 10) break;
        }
    }

    if (taprootOutputCount == 0) {
        std::cout << "No Taproot outputs found in last 100 blocks" << std::endl;
    } else {
        std::cout << "\nTotal Taproot outputs found: " << taprootOutputCount << std::endl;
    }

    return 0;
}
