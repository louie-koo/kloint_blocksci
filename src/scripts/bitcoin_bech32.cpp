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

#include "bitcoin_bech32.hpp"

namespace {

typedef std::vector<uint8_t> bech32_data;

/** The Bech32 character set for encoding. */
const char* charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

/** The Bech32 character set for decoding. */
const int8_t charset_rev[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
};

/** Checksum constants for bech32 and bech32m */
constexpr uint32_t BECH32_CONST = 1;
constexpr uint32_t BECH32M_CONST = 0x2bc830a3;

/** Concatenate two byte arrays. */
bech32_data cat(bech32_data x, const bech32_data& y) {
    x.insert(x.end(), y.begin(), y.end());
    return x;
}

/** Find the polynomial with value coefficients mod the generator as 30-bit. */
uint32_t polymod(const bech32_data& values) {
    uint32_t chk = 1;
    for (size_t i = 0; i < values.size(); ++i) {
        uint8_t top = chk >> 25;
        chk = (chk & 0x1ffffff) << 5 ^ values[i] ^
            (-((top >> 0) & 1) & 0x3b6a57b2UL) ^
            (-((top >> 1) & 1) & 0x26508e6dUL) ^
            (-((top >> 2) & 1) & 0x1ea119faUL) ^
            (-((top >> 3) & 1) & 0x3d4233ddUL) ^
            (-((top >> 4) & 1) & 0x2a1462b3UL);
    }
    return chk;
}

/** Convert to lower case. */
unsigned char lc(unsigned char c) {
    return (c >= 'A' && c <= 'Z') ? (c - 'A') + 'a' : c;
}

/** Expand a HRP for use in checksum computation. */
bech32_data expand_hrp(const std::string& hrp) {
    bech32_data ret;
    ret.resize(hrp.size() * 2 + 1);
    for (size_t i = 0; i < hrp.size(); ++i) {
        unsigned char c = hrp[i];
        ret[i] = c >> 5;
        ret[i + hrp.size() + 1] = c & 0x1f;
    }
    ret[hrp.size()] = 0;
    return ret;
}

/** Verify a checksum and determine the encoding. */
bech32::Encoding verify_checksum(const std::string& hrp, const bech32_data& values) {
    uint32_t check = polymod(cat(expand_hrp(hrp), values));
    if (check == BECH32_CONST) return bech32::Encoding::BECH32;
    if (check == BECH32M_CONST) return bech32::Encoding::BECH32M;
    return bech32::Encoding::INVALID;
}

/** Create a checksum. */
bech32_data create_checksum(const std::string& hrp, const bech32_data& values, bech32::Encoding encoding) {
    bech32_data enc = cat(expand_hrp(hrp), values);
    enc.resize(enc.size() + 6);
    uint32_t target = (encoding == bech32::Encoding::BECH32M) ? BECH32M_CONST : BECH32_CONST;
    uint32_t mod = polymod(enc) ^ target;
    bech32_data ret;
    ret.resize(6);
    for (size_t i = 0; i < 6; ++i) {
        ret[i] = (mod >> (5 * (5 - i))) & 31;
    }
    return ret;
}

} // namespace

namespace bech32 {

/** Encode a Bech32 string. */
std::string encode(const std::string& hrp, const bech32_data& values, Encoding encoding) {
    bech32_data checksum = create_checksum(hrp, values, encoding);
    bech32_data combined = cat(values, checksum);
    std::string ret = hrp + '1';
    ret.reserve(ret.size() + combined.size());
    for (size_t i = 0; i < combined.size(); ++i) {
        ret += charset[combined[i]];
    }
    return ret;
}

/** Decode a Bech32 string. */
std::tuple<std::string, bech32_data, Encoding> decode(const std::string& str) {
    bool lower = false, upper = false;
    bool ok = true;
    for (size_t i = 0; ok && i < str.size(); ++i) {
        unsigned char c = str[i];
        if (c < 33 || c > 126) ok = false;
        if (c >= 'a' && c <= 'z') lower = true;
        if (c >= 'A' && c <= 'Z') upper = true;
    }
    if (lower && upper) ok = false;
    size_t pos = str.rfind('1');
    if (ok && str.size() <= 90 && pos != str.npos && pos >= 1 && pos + 7 <= str.size()) {
        bech32_data values;
        values.resize(str.size() - 1 - pos);
        for (size_t i = 0; i < str.size() - 1 - pos; ++i) {
            unsigned char c = str[i + pos + 1];
            if (charset_rev[c] == -1) ok = false;
            values[i] = charset_rev[c];
        }
        if (ok) {
            std::string hrp;
            for (size_t i = 0; i < pos; ++i) {
                hrp += lc(str[i]);
            }
            Encoding encoding = verify_checksum(hrp, values);
            if (encoding != Encoding::INVALID) {
                return std::make_tuple(hrp, bech32_data(values.begin(), values.end() - 6), encoding);
            }
        }
    }
    return std::make_tuple(std::string(), bech32_data(), Encoding::INVALID);
}

} // namespace bech32
