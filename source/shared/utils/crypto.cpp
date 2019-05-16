#include "crypto.h"

#include "shared/utils/string.h"

#include <cryptopp/rsa.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include <QFile>

u::AesKey u::generateAesKey()
{
    AesKey key;

    CryptoPP::AutoSeededRandomPool rng;

    rng.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(key._aes), sizeof(key._aes));
    rng.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(key._iv), sizeof(key._iv));

    return key;
}

std::string u::aesDecryptBytes(const std::vector<std::byte>& bytes, const u::AesKey& aesKey)
{
    std::vector<CryptoPP::byte> outBytes(bytes.size());

    CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption decryption(reinterpret_cast<const CryptoPP::byte*>(aesKey._aes),
        sizeof(aesKey._aes), reinterpret_cast<const CryptoPP::byte*>(aesKey._iv));
    decryption.ProcessData(outBytes.data(), reinterpret_cast<const CryptoPP::byte*>(bytes.data()), bytes.size());

    return std::string(reinterpret_cast<const char*>(outBytes.data()), outBytes.size()); // NOLINT
}

std::string u::aesDecryptString(const std::string& string, const u::AesKey& aesKey)
{
    std::vector<CryptoPP::byte> outBytes(string.size());

    CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption decryption(reinterpret_cast<const CryptoPP::byte*>(aesKey._aes),
        sizeof(aesKey._aes), reinterpret_cast<const CryptoPP::byte*>(aesKey._iv));
    decryption.ProcessData(outBytes.data(), reinterpret_cast<const CryptoPP::byte*>(string.data()), string.size()); // NOLINT

    return std::string(reinterpret_cast<const char*>(outBytes.data()), outBytes.size()); // NOLINT
}

std::string u::aesEncryptString(const std::string& string, const u::AesKey& aesKey)
{
    auto inBytes = reinterpret_cast<const CryptoPP::byte*>(string.data()); // NOLINT
    auto bytesSize = string.size();

    std::vector<CryptoPP::byte> outBytes(bytesSize);

    CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption encryption(reinterpret_cast<const CryptoPP::byte*>(aesKey._aes),
        sizeof(aesKey._aes), reinterpret_cast<const CryptoPP::byte*>(aesKey._iv));
    encryption.ProcessData(outBytes.data(), inBytes, bytesSize);

    return u::bytesToHex(outBytes);
}

static CryptoPP::RSA::PublicKey loadPublicKey(const std::string& publicKeyFileName)
{
    QFile file(QString::fromStdString(publicKeyFileName));
    if(!file.open(QIODevice::ReadOnly))
        return {};

    auto byteArray = file.readAll();
    file.close();

    CryptoPP::ArraySource arraySource(reinterpret_cast<const CryptoPP::byte*>(byteArray.constData()), // NOLINT
        byteArray.size(), true);

    CryptoPP::RSA::PublicKey publicKey;
    publicKey.Load(arraySource);

    return publicKey;
}

bool u::rsaVerifySignature(const std::string& string, const std::string& signature,
    const std::string& publicKeyFileName, std::string* message)
{
    return rsaVerifySignature(signature + string, publicKeyFileName, message);
}

bool u::rsaVerifySignature(const std::string& signaturePlusString,
    const std::string& publicKeyFileName, std::string* message)
{
    CryptoPP::RSA::PublicKey publicKey = loadPublicKey(publicKeyFileName);

    CryptoPP::RSASSA_PKCS1v15_SHA_Verifier rsaVerifier(publicKey);
    std::string recoveredMessage;

    try
    {
        CryptoPP::StringSource ss(signaturePlusString, true,
            new CryptoPP::SignatureVerificationFilter(
                rsaVerifier, new CryptoPP::StringSink(recoveredMessage),
                CryptoPP::SignatureVerificationFilter::SIGNATURE_AT_BEGIN |
                CryptoPP::SignatureVerificationFilter::PUT_MESSAGE |
                CryptoPP::SignatureVerificationFilter::THROW_EXCEPTION
            ) // SignatureVerificationFilter
        ); // StringSource
        Q_UNUSED(ss);
    }
    catch(std::exception&)
    {
        return false;
    }

    if(message != nullptr)
        *message = recoveredMessage;

    return true;
}

std::string u::rsaEncryptString(const std::string& string, const std::string& publicKeyFileName)
{
    CryptoPP::RSA::PublicKey publicKey = loadPublicKey(publicKeyFileName);
    CryptoPP::RSAES_OAEP_SHA_Encryptor rsaEncryptor(publicKey);

    CryptoPP::AutoSeededRandomPool rng;
    std::vector<CryptoPP::byte> cipher(rsaEncryptor.FixedCiphertextLength());

    CryptoPP::StringSource ss(string, true,
        new CryptoPP::PK_EncryptorFilter(rng, rsaEncryptor,
            new CryptoPP::ArraySink(cipher.data(), cipher.size())
        ) // PK_EncryptorFilter
    ); // StringSource
    Q_UNUSED(ss);

    return u::bytesToHex(cipher);
}