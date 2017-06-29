#ifndef BASE64_H_
#define BASE64_H_

#include <string>

// encode to base64
std::string Base64Encode(unsigned char const* , unsigned int len);
// decode from base64
std::string Base64Dencode(std::string const& s);

#endif // BASE64_H_
