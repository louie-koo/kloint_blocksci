/* Copyright (c) 2017 Pieter Wuille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "bitcoin_segwit_addr.hpp"

#include "bitcoin_bech32.hpp"

#include <internal/data_configuration.hpp>

namespace {

typedef std::vector<uint8_t> segwit_data;

/** Convert from one power-of-2 number base to another. */
template<int frombits, int tobits, bool pad>
bool convertbits(segwit_data& out, const segwit_data& in) {
    int acc = 0;
    int bits = 0;
    const int maxv = (1 << tobits) - 1;
    const int max_acc = (1 << (frombits + tobits - 1)) - 1;
    for (size_t i = 0; i < in.size(); ++i) {
        int value = in[i];
        acc = ((acc << frombits) | value) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            out.push_back((acc >> bits) & maxv);
        }
    }
    if (pad) {
        if (bits) out.push_back((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}

} // namespace

namespace segwit_addr {

/** Decode a SegWit address. */
std::pair<int, segwit_data> decode(const std::string& hrp, const std::string& addr) {
    auto [dec_hrp, dec_data, encoding] = bech32::decode(addr);
    if (dec_hrp != hrp || dec_data.size() < 1 || encoding == bech32::Encoding::INVALID) {
        return std::make_pair(-1, segwit_data());
    }

    int witver = dec_data[0];

    // Version 0 must use bech32, version 1+ must use bech32m
    if (witver == 0 && encoding != bech32::Encoding::BECH32) {
        return std::make_pair(-1, segwit_data());
    }
    if (witver != 0 && encoding != bech32::Encoding::BECH32M) {
        return std::make_pair(-1, segwit_data());
    }

    segwit_data conv;
    if (!convertbits<5, 8, false>(conv, segwit_data(dec_data.begin() + 1, dec_data.end())) ||
        conv.size() < 2 || conv.size() > 40 || witver > 16 || (witver == 0 &&
        conv.size() != 20 && conv.size() != 32)) {
        return std::make_pair(-1, segwit_data());
    }
    return std::make_pair(witver, conv);
}

/** Encode a SegWit address. */
std::string encode(const std::string& hrp, int witver, const segwit_data& witprog) {
    segwit_data enc;
    enc.push_back(static_cast<unsigned char>(witver));
    convertbits<8, 5, true>(enc, witprog);

    // Version 0 uses bech32, version 1+ uses bech32m
    bech32::Encoding encoding = (witver == 0) ? bech32::Encoding::BECH32 : bech32::Encoding::BECH32M;
    std::string ret = bech32::encode(hrp, enc, encoding);

    if (decode(hrp, ret).first == -1) return "";
    return ret;
}

std::string encode(const blocksci::ChainConfiguration &config, int witver, const segwit_data& witprog) {
    segwit_data enc;
    enc.push_back(static_cast<unsigned char>(witver));
    convertbits<8, 5, true>(enc, witprog);

    // Version 0 uses bech32, version 1+ uses bech32m
    bech32::Encoding encoding = (witver == 0) ? bech32::Encoding::BECH32 : bech32::Encoding::BECH32M;
    std::string ret = bech32::encode(config.segwitPrefix, enc, encoding);

    if (decode(config.segwitPrefix, ret).first == -1) return "";
    return ret;
}

} // namespace segwit_addr
