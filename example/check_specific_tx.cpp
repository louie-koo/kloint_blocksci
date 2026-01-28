//
//  check_specific_tx.cpp
//  Check specific TX and its outputs
//

#include <blocksci/blocksci.hpp>
#include <blocksci/chain/transaction.hpp>
#include <blocksci/chain/block.hpp>
#include <blocksci/chain/access.hpp>
#include <blocksci/scripts/witness_unknown_script.hpp>

#include <internal/data_access.hpp>
#include <internal/hash_index.hpp>

#include <iostream>
#include <string>

using namespace blocksci;

int main(int argc, const char * argv[]) {
    std::string configPath = "/home/kloint/blocksci_data/config.json";

    // TX 98a750ceb167fef82c62e9c2e546fcff00031d11204c81b64bbcab5ffcee5a44 is in block 930940
    int blockHeight = 930940;
    std::string targetTxHash = "98a750ceb167fef82c62e9c2e546fcff00031d11204c81b64bbcab5ffcee5a44";
    std::string targetAddr = "bc1pmanxyggsck006vg8wayvlltyffvwwpfpcpv8qsqj0tg5fml7hmmsc7zfsj";

    std::cout << "=== Check Specific TX ===" << std::endl;
    std::cout << "Looking for TX: " << targetTxHash << std::endl;
    std::cout << "In block: " << blockHeight << std::endl;

    Blockchain chain(configPath);
    auto &access = chain.getAccess();

    // Get target address info first
    auto addrOpt = getAddressFromString(targetAddr, access);
    if (!addrOpt) {
        std::cout << "Target address NOT FOUND in BlockSci: " << targetAddr << std::endl;
        return 1;
    }
    std::cout << "Target address scriptNum: " << addrOpt->scriptNum << ", type: " << static_cast<int>(addrOpt->type) << std::endl;

    // Search in block
    Block block = chain[blockHeight];
    std::cout << "\nBlock " << blockHeight << " has " << block.size() << " transactions" << std::endl;

    bool found = false;
    for (auto tx : block) {
        std::string txHashHex = tx.getHash().GetHex();
        if (txHashHex == targetTxHash) {
            found = true;
            std::cout << "\nTX FOUND! txNum=" << tx.txNum << std::endl;
            std::cout << "Outputs: " << tx.outputCount() << std::endl;

            std::cout << "\n=== Outputs ===" << std::endl;
            int idx = 0;
            bool hasTargetAddr = false;
            for (auto output : tx.outputs()) {
                auto addr = output.getAddress();
                std::cout << "[" << idx << "] type=" << static_cast<int>(addr.type)
                          << ", scriptNum=" << addr.scriptNum
                          << ", value=" << output.getValue() << " sat";

                if (addr.type == AddressType::WITNESS_UNKNOWN) {
                    ScriptAddress<AddressType::WITNESS_UNKNOWN> script(addr.scriptNum, access);
                    std::cout << "\n     Address: " << script.addressString();
                }

                if (addr.scriptNum == addrOpt->scriptNum && addr.type == addrOpt->type) {
                    std::cout << " <-- TARGET ADDRESS!";
                    hasTargetAddr = true;
                }
                std::cout << std::endl;
                idx++;
            }

            if (!hasTargetAddr) {
                std::cout << "\n*** WARNING: Target address NOT found in this TX outputs!" << std::endl;
            }
            break;
        }
    }

    if (!found) {
        std::cout << "TX NOT FOUND in block " << blockHeight << std::endl;
    }

    return 0;
}
