//
//  taproot_tx_list.cpp
//  List all transactions for a specific Taproot address
//

#include <blocksci/blocksci.hpp>
#include <blocksci/chain/transaction.hpp>
#include <blocksci/chain/block.hpp>
#include <blocksci/chain/access.hpp>
#include <blocksci/address/address.hpp>
#include <blocksci/script.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>

#include <internal/data_access.hpp>
#include <internal/hash_index.hpp>

#include <iostream>
#include <string>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";
    std::string targetAddress = "bc1pmanxyggsck006vg8wayvlltyffvwwpfpcpv8qsqj0tg5fml7hmmsc7zfsj";

    if (argc > 1) {
        targetAddress = argv[1];
    }

    std::cout << "=== Taproot Address TX List ===" << std::endl;
    std::cout << "Target: " << targetAddress << std::endl;

    Blockchain chain(configPath);
    auto &access = chain.getAccess();

    std::cout << "Chain loaded: " << chain.size() << " blocks" << std::endl;

    // Look up address
    auto addrOpt = getAddressFromString(targetAddress, access);
    if (!addrOpt) {
        std::cerr << "Address NOT FOUND in BlockSci data" << std::endl;
        return 1;
    }

    Address addr = *addrOpt;
    std::cout << "Address type: " << static_cast<int>(addr.type) << " (WITNESS_UNKNOWN)" << std::endl;
    std::cout << "Script num: " << addr.scriptNum << std::endl;

    // Get all transactions
    std::cout << "\n=== Transaction List ===" << std::endl;

    auto txes = addr.getTransactions();
    int txCount = 0;

    for (auto tx : txes) {
        txCount++;
        std::cout << "\nTX #" << txCount << ":" << std::endl;
        std::cout << "  Hash: " << tx.getHash().GetHex() << std::endl;
        std::cout << "  Block: " << tx.block().height() << std::endl;
        std::cout << "  TxNum: " << tx.txNum << std::endl;
        std::cout << "  Inputs: " << tx.inputCount() << ", Outputs: " << tx.outputCount() << std::endl;

        // Show inputs
        std::cout << "  Inputs:" << std::endl;
        int inIdx = 0;
        for (auto input : tx.inputs()) {
            auto inAddr = input.getAddress();
            std::cout << "    [" << inIdx << "] value=" << input.getValue()
                      << " sat, type=" << static_cast<int>(inAddr.type) << std::endl;
            inIdx++;
        }

        // Show outputs
        std::cout << "  Outputs:" << std::endl;
        int outIdx = 0;
        for (auto output : tx.outputs()) {
            auto outAddr = output.getAddress();
            std::cout << "    [" << outIdx << "] value=" << output.getValue()
                      << " sat, type=" << static_cast<int>(outAddr.type);
            if (outAddr.scriptNum == addr.scriptNum && outAddr.type == addr.type) {
                std::cout << " <-- THIS ADDRESS";
            }
            std::cout << std::endl;
            outIdx++;
        }
    }

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Total transactions: " << txCount << std::endl;

    // Compare with Blockstream
    std::cout << "\nVerify on Blockstream:" << std::endl;
    std::cout << "https://blockstream.info/address/" << targetAddress << std::endl;

    return 0;
}
