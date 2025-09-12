#include "maxon/big_integer.h"
#include "maxon/crc32c.h"
#include "maxon/cryptography.h"
#include "maxon/datacompression.h"
#include "maxon/errortypes.h"
#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

namespace maxon
{

class PGPTest : public CommandData
{
	static const Int keySize = 256, blockSize = 256;
#ifdef MAXON_TARGET_DEBUG
	// lower value for speed reasons
	static const Int certainty = 20;
#else
	static const Int certainty = 300;
#endif

public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager)
	{
		return DoPGPTest() == OK;
	}

	Result<void> DoPGPTest()
	{
		static constexpr const Int ORIG_BUFFER_SIZE = 1 << 20;	// 1 MiB
		BaseArray<UChar> data;

		// set up our original data
		iferr (data.Resize(ORIG_BUFFER_SIZE))
			return err;
		SecureRandomProvider provider = SecureRandom::GetDefaultProvider();
		SecureRandom::GetRandomNumber(provider, data);
		// Only use numbers letters and spaces so that compression actually does something. With good random numbers the entropy of the buffer is 1 and there is nothing to do.
		for (Int j = 0; j < data.GetCount(); j++)
		{
			data[j] &= 127;
			if (!IsAlphanumeric(data[j]))
				data[j] = ' ';
		}

		Crc32C crc;
		crc.Update(data);
		UInt32 origCRC = crc.GetCrc();

		BigInteger publicKeyReceiver, privateKeyReceiver, nReceiver, encryptedSymmetricKey;
		BigInteger publicKeySender, privateKeySender, nSender, signature;

		// Generate a public (publicKeyReceiver, nReceiver) and a private (privateKeyReceiver, nReceiver) key pair for the receiver.
		iferr (GenerateAsymmetricKeys(publicKeyReceiver, privateKeyReceiver, nReceiver))
			return err;

		// Generate a public (publicKeySender, nSender) and a private (privateKeySender, nSender) key pair for the sender.
		iferr (GenerateAsymmetricKeys(publicKeySender, privateKeySender, nSender))
			return err;

		// Encrypt the data using the receiver's public key and create a symmetric key for encryption.
		Block<UChar> transmitData;
		iferr (EncryptData(data, transmitData, publicKeyReceiver, nReceiver, encryptedSymmetricKey))
			return err;

		// Create a signature so that the receiver can be sure that the data originally came from the sender.
		iferr (CreateSignature(data, privateKeySender, nSender, signature))
			return err;

		// Now, send transmitData, encryptedSymmetricKey and signature to the receiver.

		// The receiver decrypts the data using the receiver's private key.
		Block<UChar> receiveData;
		iferr (DecryptData(transmitData, receiveData, privateKeyReceiver, nReceiver, encryptedSymmetricKey))
			return err;

		// Let's check the signature.
		Bool signatureOK1 = false, signatureOK2 = false;
		iferr (CheckSignature(receiveData.GetFirst(), receiveData.GetCount(), publicKeySender, nSender, signature, signatureOK1))
			return err;

		UChar* mem = transmitData.GetFirst();
		DeleteMem(mem);
		transmitData.Reset();

		crc.Reset();
		crc.Update(receiveData);
		UInt32 newCRC = crc.GetCrc();

		// Modify the received data and check the signature again. It must not match now.
		receiveData[0]++;
		iferr (CheckSignature(receiveData.GetFirst(), receiveData.GetCount(), publicKeySender, nSender, signature, signatureOK2))
			return err;

		DeleteConstPtrMem(receiveData.GetFirst());

		Bool ok = (origCRC == newCRC && signatureOK1 && !signatureOK2);
		::String error;
		if (ok)
		{
			error = "PGP test succeeded";
		}
		else
		{
			error = "PGP test failed: ";
			if (origCRC != newCRC)
				error += "CRC different ";
			if (!signatureOK1)
				error += "Signature 1 mismatch ";
			if (signatureOK2)
				error += "Signature 2 mismatch ";
		}
		GeOutString(error, GEMB::OK);

		return OK;
	}

	Result<void> GenerateAsymmetricKeys(BigInteger& publicKey, BigInteger& privateKey, BigInteger& n)
	{
		iferr_scope;

		SecureRandomProvider provider = SecureRandom::GetDefaultProvider();
		BigInteger p, q, phi_n, i, one;

		// Generate a key pair for asymmetric encryption and decryption. These numbers are just created here randomly.
		// Normally, they are only created once, the result is stored and p and q are forgotten and destroyed. No one may ever get knowledge about p or q.
		// p and q can be 2**128 - 1 at maximum. Their product is less than 2**256 so that they can be used for a 256-bit key.
		p.SetRandomPrime(128, certainty, provider) iferr_return;
		q.SetRandomPrime(128, certainty, provider) iferr_return;
		n = p * q;
		privateKey.SetRandomPrime(129, certainty, provider) iferr_return;	// take 129 bis here to make sure it's larger than max(p, q).
		one.Set((UChar)1) iferr_return;
		p.Dec() iferr_return;
		q.Dec() iferr_return;
		phi_n = p * q;

		publicKey = privateKey;
		publicKey.MultiplicativeInverse(phi_n) iferr_return;

		// just a test: a number multiplied with its inverse mod n must be 1.
		i = publicKey;
		i.Mul(privateKey) iferr_return;
		i.Mod(phi_n) iferr_return;
		DebugAssert(i.Compare(one) == COMPARERESULT::EQUAL);

		return OK;
	}

	Result<void> EncryptData(const Block<const UChar>& origData, Block<UChar>& transmitData, const BigInteger& publicKey, const BigInteger& n, BigInteger& encryptedSymmetricKey)
	{
		SecureRandomProvider provider;
		BaseArray<UChar> compressedMem;
		BigInteger symmetricKey;

		iferr (StreamConversionRef uncompress = StreamConversions::ZipEncoder().Create())
			return err;

		provider = SecureRandom::GetDefaultProvider();

		// generate a key for the symmetric encryption and decryption. This must be smaller than n. A number mod n will always be >= 0 and < n.
		do
		{
			iferr (symmetricKey.SetRandom(256))
				return err;
		} while (symmetricKey.Compare(n) != COMPARERESULT::LESS);

		// encrypt this symmetric key using our private asymmetrical key pair.
		encryptedSymmetricKey = symmetricKey;
		iferr (encryptedSymmetricKey.PowMod(publicKey, n))
			return err;

		// compress our data and make the size a multiple of the (block size + 2*8) bytes for the size information. The block size is usually given in bits for cryptography.
		iferr (compressedMem.Resize(2 * sizeof(UInt64), COLLECTION_RESIZE_FLAGS::POD_UNINITIALIZED))
			return err;
		iferr (uncompress.ConvertAll(origData, compressedMem))
			return err;

		Int blocks = blockSize / 8;

		Int unpaddedCompressedSize = compressedMem.GetCount();

		iferr (compressedMem.Resize((unpaddedCompressedSize + blocks - 1) & ~(blocks - 1)))
			return err;

		*(UInt64*)&compressedMem[0 * sizeof(UInt64)] = (UInt64)origData.GetCount();
		*(UInt64*)&compressedMem[1 * sizeof(UInt64)] = (UInt64)unpaddedCompressedSize;

		// fill the memory up to the block boundary with random data to increase security.
		if (unpaddedCompressedSize < compressedMem.GetCount())
			SecureRandom::GetRandomNumber(provider, ToBlock<UChar>(&compressedMem[unpaddedCompressedSize], compressedMem.GetCount() - unpaddedCompressedSize));

		BaseArray<UChar> keyArray;

		// now encrypt everything with the symmetrical key
		iferr (symmetricKey.GetDataCopy(true, keyArray))
			return err;

		DebugAssert(keyArray.GetCount() <= keySize / 8);

		iferr (keyArray.Resize(keySize / 8))
			return err;

		DataDictionary settings;
		iferr (settings.Set(CryptographyOptions::CRYPTOKEY, CryptoKey(Id(), blockSize, keyArray.GetFirst(), keyArray.GetCount() * 8)))
			return err;
		iferr (StreamConversionRef crypt = StreamConversions::AesEncoder().Create(settings))
			return err;

		iferr (crypt.ConvertAllInplace(compressedMem))
			return err;

		transmitData = compressedMem.Disconnect();

		return OK;
	}

	Result<void> DecryptData(Block<UChar>& transmitData, Block<UChar>& receiveData, const BigInteger& privateKey, const BigInteger& n, const BigInteger& encryptedSymmetricKey)
	{
		iferr (StreamConversionRef uncompress = StreamConversions::ZipDecoder().Create())
			return err;

		BigInteger symmetricKey;

		// decrypt the symmetric key
		symmetricKey = encryptedSymmetricKey;
		iferr (symmetricKey.PowMod(privateKey, n))
			return err;

		// decrypt the data using the symmetric key
		BaseArray<UChar> keyArray;
		iferr (symmetricKey.GetDataCopy(true, keyArray))
			return err;
		DebugAssert(keyArray.GetCount() <= keySize / 8);

		iferr (keyArray.Resize(keySize / 8))
			return err;

		DataDictionary settings;
		iferr (settings.Set(CryptographyOptions::CRYPTOKEY, CryptoKey(Id(), blockSize, keyArray.GetFirst(), keyArray.GetCount() * 8)))
			return err;
		iferr (StreamConversionRef decrypt = StreamConversions::AesDecoder().Create(settings))
			return err;

		iferr (decrypt.ConvertAllInplace(ToBlock(transmitData.GetFirst(), transmitData.GetCount())))
			return err;

		// extract size information
		// origBufferSize = *static_cast<UInt64*>((transmitData + 0));
		UInt64 compressedDataSize = *(UInt64*)&transmitData[sizeof(UInt64)];

		// uncompress
		BaseArray<UChar> uncompressedMem;
		iferr (uncompress.ConvertAll(transmitData.Slice(2 * sizeof(UInt64), (Int)compressedDataSize), uncompressedMem))
			return err;

		if (!uncompressedMem.GetFirst() || !uncompressedMem.GetCount())
			return UnknownError(MAXON_SOURCE_LOCATION, String());

		receiveData = uncompressedMem.Disconnect();

		return OK;
	}

	Result<void> CreateSignature(const Block<const UChar>& data, const BigInteger& privateKeySender, const BigInteger& nSender, BigInteger& signature)
	{
		// The signature is a hash value of the data that is encrypted with the sender's private key. We take the CRC value here for simplicity.
		Crc32C crc;
		crc.Update(data);
		iferr (signature.Set(crc.GetCrc()))
			return err;
		iferr (signature.PowMod(privateKeySender, nSender))
			return err;

		return OK;
	}

	Result<void> CheckSignature(const void* data, Int dataSize, const BigInteger& publicKeySender, const BigInteger& nSender, const BigInteger& signature, Bool& signatureOK)
	{
		// The signature is a hash value of the data that is encrypted with the sender's private key. We take the CRC value here for simplicity.
		BigInteger signatureTest = signature;
		Crc32C crc;
		crc.Update(ToBlock(data, dataSize));

		BigInteger hash;
		iferr (hash.Set(crc.GetCrc()))
			return err;

		iferr (signatureTest.PowMod(publicKeySender, nSender))
			return err;
		signatureOK = signatureTest.Compare(hash) == COMPARERESULT::EQUAL;

		return OK;
	}
};

}
/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_PGPTEST = 450000266;

Bool RegisterPGPTest()
{
	// be sure to use a unique ID obtained from developers.maxon.net
	return RegisterCommandPlugin(ID_PGPTEST, GeLoadString(IDS_PGPTEST), 0, nullptr, String("C++ SDK PGP test"), NewObjClear(maxon::PGPTest));
}
