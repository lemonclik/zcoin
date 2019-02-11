#include "Scalar.h"

#include "include/secp256k1.h"

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include "../scalar.h"
#include "../scalar_impl.h"
#include "../hash_impl.h"
#include "../hash.h"
#include "../../crypto/sha256.h"
#include <sstream>
#include <iostream>
#include <openssl/rand.h>

namespace secp_primitives {

Scalar::Scalar()
   : value_(new secp256k1_scalar()) {
}

Scalar::Scalar(uint64_t value)
   : value_(new secp256k1_scalar()) {
    secp256k1_scalar_set_int(reinterpret_cast<secp256k1_scalar *>(value_), value);
}

Scalar::Scalar(const char* str)
     : value_(new secp256k1_scalar()) {
    const unsigned char* str_ = reinterpret_cast<const unsigned char *>( &str );
    secp256k1_scalar_set_b32(reinterpret_cast<secp256k1_scalar *>(value_),str_,0);
}

Scalar::Scalar(const void *value)
   : value_(new secp256k1_scalar(*reinterpret_cast<const secp256k1_scalar *>(value))) {

}

Scalar::Scalar(const Scalar& other)
   : value_(new secp256k1_scalar(*reinterpret_cast<const secp256k1_scalar *>(other.value_))) {

}

Scalar::~Scalar() {
    delete reinterpret_cast<secp256k1_scalar *>(value_);
}

Scalar& Scalar::operator=(const Scalar& other) {
    return set(other);
}

Scalar& Scalar::operator=(unsigned int i) {
    secp256k1_scalar_set_int(reinterpret_cast<secp256k1_scalar *>(value_), i);
    return *this;
}

Scalar& Scalar::operator=(const unsigned char *bin){
    secp256k1_scalar_set_b32(reinterpret_cast<secp256k1_scalar *>(value_), bin, NULL);
    return *this;
}

Scalar& Scalar::set(const Scalar& other) {
    *reinterpret_cast<secp256k1_scalar *>(value_) = *reinterpret_cast<const secp256k1_scalar *>(other.value_);
    return *this;
}

Scalar Scalar::operator*(const Scalar& other) const {
    secp256k1_scalar result;
    secp256k1_scalar_mul(&result, reinterpret_cast<const secp256k1_scalar *>(value_), reinterpret_cast<const secp256k1_scalar *>(other.value_));
    return &result;
}

Scalar& Scalar::operator*=(const Scalar& other) {
    secp256k1_scalar_mul(reinterpret_cast<secp256k1_scalar *>(value_), &(*reinterpret_cast<const secp256k1_scalar *>(value_)), reinterpret_cast<const secp256k1_scalar *>(other.value_));
    return *this;
}

Scalar Scalar::operator+(const Scalar& other) const {
    secp256k1_scalar result;
    secp256k1_scalar_add(&result, reinterpret_cast<const secp256k1_scalar *>(value_), reinterpret_cast<const secp256k1_scalar *>(other.value_));
    return &result;
}

Scalar& Scalar::operator+=(const Scalar& other) {
    secp256k1_scalar_add(reinterpret_cast<secp256k1_scalar *>(value_), reinterpret_cast<const secp256k1_scalar *>(value_), reinterpret_cast<const secp256k1_scalar *>(other.value_));
    return *this;
}

Scalar Scalar::operator-(const Scalar& other) const {
    secp256k1_scalar result;
    secp256k1_scalar_negate(&result,reinterpret_cast<const secp256k1_scalar *>(other.value_));
    secp256k1_scalar_add(&result,&result,reinterpret_cast<const secp256k1_scalar *>(value_));
    return &result;
}

Scalar& Scalar::operator-=(const Scalar& other) {
    secp256k1_scalar temp;
    secp256k1_scalar_negate(&temp,reinterpret_cast<const secp256k1_scalar *>(other.value_));
    secp256k1_scalar_add(reinterpret_cast<secp256k1_scalar *>(value_),reinterpret_cast<const secp256k1_scalar *>(value_),&temp);
    return *this;
}

bool Scalar::operator==(const Scalar& other)const {
    return secp256k1_scalar_eq(reinterpret_cast<const secp256k1_scalar *>(value_), reinterpret_cast<const secp256k1_scalar *>(other.value_));
}

bool Scalar::operator!=(const Scalar& other)const {
    return !(secp256k1_scalar_eq(reinterpret_cast<const secp256k1_scalar *>(value_), reinterpret_cast<const secp256k1_scalar *>(other.value_)));
}

const void * Scalar::get_value() const {
    return value_;
}

Scalar Scalar::inverse() const {
    secp256k1_scalar result;
    secp256k1_scalar_inverse(&result, reinterpret_cast<const secp256k1_scalar *>(value_));
 return &result;
}

Scalar Scalar::negate() const {
    secp256k1_scalar result;
    secp256k1_scalar_negate(&result, reinterpret_cast<const secp256k1_scalar *>(value_));
    return &result;
}

Scalar Scalar::square() const{
    secp256k1_scalar result;
    secp256k1_scalar_sqr(&result, reinterpret_cast<const secp256k1_scalar *>(value_));
 return &result;
}

Scalar Scalar::exponent(const Scalar& exp) const {
    secp256k1_scalar result;
    secp256k1_scalar value = *reinterpret_cast<const secp256k1_scalar *>(value_);
    secp256k1_scalar_set_int(&result, 1);
    secp256k1_scalar exp_ = *reinterpret_cast<const secp256k1_scalar *>(exp.value_);
    while (!secp256k1_scalar_is_zero(&exp_))
    {
       if(!secp256k1_scalar_is_even(&exp_))
           secp256k1_scalar_mul(&result,&result, &value);
       secp256k1_scalar_sqr(&value, &value);
       secp256k1_scalar_shr_int(&exp_,1);

    }
     return &result;
}

Scalar Scalar::exponent(uint64_t exp) const {
    Scalar exp_(exp);
    return exponent(exp_);

}

bool Scalar::isMember() const{
    Scalar temp(*this);
    temp.mod_p();
     return *this == temp;
    }

Scalar& Scalar::randomize(){
    unsigned char temp[32] = { 0 };
    do {
        if (RAND_bytes(temp, 32) != 1) {
            throw "Unable to generate random Scalar";
        }
        generate(temp);
    }while (!(this->isMember()));
}
Scalar& Scalar::generate(unsigned char* buff){
    secp256k1_scalar_set_b32(reinterpret_cast<secp256k1_scalar *>(value_), buff, NULL);
    secp256k1_scalar zero;
     secp256k1_scalar_set_int(&zero, 0);
     secp256k1_scalar_add(reinterpret_cast<secp256k1_scalar *>(value_),reinterpret_cast<const secp256k1_scalar *>(value_),&zero);
     return *this;
    }

    Scalar& Scalar::mod_p() {
    secp256k1_scalar zero;
    secp256k1_scalar_clear(&zero);
    secp256k1_scalar_add(reinterpret_cast<secp256k1_scalar *>(value_),reinterpret_cast<const secp256k1_scalar *>(value_),&zero);
    return *this;
}

Scalar Scalar::hash(const unsigned char* data,size_t len)  {
    unsigned char hash[CSHA256::OUTPUT_SIZE];
    CSHA256().Write(data,len).Finalize(hash);

    int overflow = 0;
    secp256k1_scalar result;
    secp256k1_scalar_set_b32(&result,hash,&overflow);
    if (overflow) {
     throw "Scalar: hashing overflowed";
    }
    Scalar result_(&result);
    result_.mod_p();
    return result_;
}


std::string Scalar::tostring() const {
    unsigned char buffer[32];
    secp256k1_scalar_get_b32(buffer,reinterpret_cast<const secp256k1_scalar *>(value_));
    std::stringstream ss;
    for(int i = 0; i < 32; ++i){
        ss <<  (int)buffer[i];
    }

return ss.str();
}

size_t Scalar::memoryRequired() const  {
    return 32;
}

unsigned char* Scalar::serialize(unsigned char* buffer) const {
    secp256k1_scalar_get_b32(buffer,reinterpret_cast<const secp256k1_scalar *>(value_));
return buffer + 32;
}

unsigned char* Scalar::deserialize(unsigned char* buffer)  {
    int overflow = 0;
    secp256k1_scalar_set_b32(reinterpret_cast<secp256k1_scalar *>(value_),buffer,&overflow);
    if (overflow) {
        throw "Scalar: decoding overflowed";
    }
    return buffer + 32;
}

std::string Scalar::GetHex() const{
    unsigned char buffer[32];
    serialize(buffer);
    std::stringstream ss;
    for(int i = 0; i < 32; ++i)
    {
        ss << buffer[i] / 16;
        ss << buffer[i] % 16;
    }

}

void Scalar::SetHex(const std::string& str) const{
    unsigned char buffer[32];
    for(int i = 0; i < 32; i+=2)
        buffer[i] =  str[i] * 16 + str[i + 1];
    int overflow = 0;
    secp256k1_scalar_set_b32(reinterpret_cast<secp256k1_scalar *>(value_),buffer,&overflow);
    if (overflow) {
        throw "Scalar: decoding overflowed";
    }
}

void Scalar::get_bits(std::vector<bool>& bits) const{
    unsigned char bin[32];
    secp256k1_scalar_get_b32(bin, reinterpret_cast<const secp256k1_scalar *>(value_));
    for (int i = 0; i < 32; ++i) {
        int32_t val = bin[i];
        for (int j = 7; j >= 0; j--) {
            bits.push_back((val >> j) & 1);
        }
    }
}

} // namespace secp_primitives
