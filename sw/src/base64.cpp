#include "common.h"
#include "base64.h"

static const std::string base64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
    "abcdefghijklmnopqrstuvwxyz" \
    "0123456789+/";

static inline bool IsBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Base64Encode(unsigned char const* BytesToEncode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char charArray3[3];
    unsigned char charArray4[4];

    while (in_len--) {
        charArray3[i++] = *(BytesToEncode++);
        if (i == 3) {
            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4)
                + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2)
                + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64Chars[charArray4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            charArray3[j] = '\0';

        charArray4[0] = (charArray3[0] & 0xfc) >> 2;
        charArray4[1] = ((charArray3[0] & 0x03) << 4)
            + ((charArray3[1] & 0xf0) >> 4);
        charArray4[2] = ((charArray3[1] & 0x0f) << 2)
            + ((charArray3[2] & 0xc0) >> 6);
        charArray4[3] = charArray3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64Chars[charArray4[j]];

        while ((i++ < 3))
            ret += '=';
    }
    return ret;
}

std::string Base64Dencode(std::string const& EncodedString) {
    size_t in_len = EncodedString.size();
    size_t i = 0;
    size_t j = 0;
    int in_ = 0;
    unsigned char charArray4[4], charArray3[3];
    std::string ret;

    while (in_len-- && (EncodedString[in_] != '=')
        && IsBase64(EncodedString[in_])) {
        charArray4[i++] = EncodedString[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                charArray4[i] = static_cast<unsigned char>(base64Chars.find(
                    charArray4[i]));

            charArray3[0] = (charArray4[0] << 2)
                + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4)
                + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

            for (i = 0; (i < 3); i++)
                ret += charArray3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            charArray4[j] = 0;

        for (j = 0; j < 4; j++)
            charArray4[j] = static_cast<unsigned char>(base64Chars.find(
                charArray4[j]));

        charArray3[0] = (charArray4[0] << 2)
            + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4)
            + ((charArray4[2] & 0x3c) >> 2);
        charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

        for (j = 0; (j < i - 1); j++)
            ret += charArray3[j];
    }

    return ret;
}

