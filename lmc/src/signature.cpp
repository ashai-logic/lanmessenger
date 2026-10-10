/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
**
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
** Modified by ASHAI, 2026.
**
****************************************************************************/


#include <QFile>
#include <QCoreApplication>
#include <cstring>
#include <cstdlib>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include "definitions.h"
#include "signature.h"

static bool nextPacket(const QByteArray& b, int& o, int& tag, int& body, int& len) {
	int n = b.size();
	if(o >= n)
		return false;
	unsigned char ctb = (unsigned char)b[o];
	if(!(ctb & 0x80))
		return false;
	if(ctb & 0x40) {
		tag = ctb & 0x3f;
		o++;
		if(o >= n)
			return false;
		unsigned char x = (unsigned char)b[o++];
		if(x < 192)
			len = x;
		else if(x < 224) {
			if(o >= n)
				return false;
			len = ((x - 192) << 8) + (unsigned char)b[o++] + 192;
		} else if(x == 255) {
			if(o + 4 > n)
				return false;
			len = ((unsigned char)b[o] << 24) | ((unsigned char)b[o + 1] << 16) |
				  ((unsigned char)b[o + 2] << 8) | (unsigned char)b[o + 3];
			o += 4;
		} else {
			return false;
		}
	} else {
		tag = (ctb >> 2) & 0x0f;
		int lt = ctb & 0x03;
		o++;
		if(lt == 0) {
			if(o >= n)
				return false;
			len = (unsigned char)b[o];
			o += 1;
		} else if(lt == 1) {
			if(o + 2 > n)
				return false;
			len = ((unsigned char)b[o] << 8) | (unsigned char)b[o + 1];
			o += 2;
		} else if(lt == 2) {
			if(o + 4 > n)
				return false;
			len = ((unsigned char)b[o] << 24) | ((unsigned char)b[o + 1] << 16) |
				  ((unsigned char)b[o + 2] << 8) | (unsigned char)b[o + 3];
			o += 4;
		} else {
			return false;
		}
	}
	body = o;
	if(body + len > n)
		return false;
	o = body + len;
	return true;
}

QByteArray lmcSignature::base64Decode(const QByteArray& data) {
	static const char* alphabet =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int table[256];
	for(int i = 0; i < 256; i++)
		table[i] = -1;
	for(int i = 0; i < 64; i++)
		table[(unsigned char)alphabet[i]] = i;

	QByteArray out;
	int val = 0;
	int valb = -8;
	for(int i = 0; i < data.size(); i++) {
		char c = data.at(i);
		if(c == '=')
			break;
		int d = table[(unsigned char)c];
		if(d < 0)
			continue;
		val = (val << 6) + d;
		valb += 6;
		if(valb >= 0) {
			out.append((char)((val >> valb) & 0xff));
			valb -= 8;
		}
	}
	return out;
}

QByteArray lmcSignature::dearmor(const QByteArray& data) {
	if(!data.contains("-----BEGIN"))
		return data;
	QByteArray ascii = data;
	ascii.replace("\r", "");
	int begin = ascii.indexOf("-----BEGIN");
	int bstart = ascii.indexOf("\n\n", begin);
	if(bstart < 0)
		return QByteArray();
	bstart += 2;
	int end = ascii.indexOf("-----END", bstart);
	if(end < 0)
		return QByteArray();
	return base64Decode(ascii.mid(bstart, end - bstart));
}

bool lmcSignature::parsePublicKey(const QByteArray& keyData, unsigned char* rawKey) {
	int o = 0, tag = 0, body = 0, len = 0;
	while(nextPacket(keyData, o, tag, body, len)) {
		if(tag != 6)
			continue;
		const unsigned char* p = (const unsigned char*)keyData.constData() + body;
		if(len < 7 || p[0] != 4)
			continue;
		int algo = p[5];
		if(algo != 22)	//	22 = EdDSA (Ed25519)
			return false;
		int q = 6;
		int oidLen = p[q];
		q += 1 + oidLen;
		if(q + 2 > len)
			return false;
		int bits = (p[q] << 8) | p[q + 1];
		q += 2;
		int nb = (bits + 7) / 8;
		if(q + nb > len)
			return false;
		if(nb == 33 && p[q] == 0x40) {
			memcpy(rawKey, p + q + 1, 32);
			return true;
		}
		if(nb == 32) {
			memcpy(rawKey, p + q, 32);
			return true;
		}
		return false;
	}
	return false;
}

bool lmcSignature::parseSignature(const QByteArray& sigData, unsigned char* sig, int* pHashAlgo,
								  QByteArray& hashedData) {
	int o = 0, tag = 0, body = 0, len = 0;
	while(nextPacket(sigData, o, tag, body, len)) {
		if(tag != 2)
			continue;
		const unsigned char* p = (const unsigned char*)sigData.constData() + body;
		if(len < 10 || p[0] != 4)
			continue;
		int pubAlgo = p[2];
		if(pubAlgo != 22)	//	22 = EdDSA (Ed25519)
			continue;
		*pHashAlgo = p[3];
		int hlen = (p[4] << 8) | p[5];
		if(6 + hlen > len)
			continue;
		hashedData = QByteArray((const char*)p, 6 + hlen);
		int off = 6 + hlen;
		if(off + 2 > len)
			continue;
		int uhlen = (p[off] << 8) | p[off + 1];
		off += 2 + uhlen;
		if(off + 2 > len)
			continue;
		off += 2;	//	left 16 bits of the signed hash
		if(off + 2 > len)
			continue;
		int rb = (p[off] << 8) | p[off + 1];
		off += 2;
		int rn = (rb + 7) / 8;
		if(off + rn > len)
			continue;
		const unsigned char* r = p + off;
		off += rn;
		if(off + 2 > len)
			continue;
		int sb = (p[off] << 8) | p[off + 1];
		off += 2;
		int sn = (sb + 7) / 8;
		if(off + sn > len)
			continue;
		const unsigned char* s = p + off;
		memset(sig, 0, 64);
		if(rn > 32 || sn > 32)
			continue;
		memcpy(sig + (32 - rn), r, rn);
		memcpy(sig + 32 + (32 - sn), s, sn);
		return true;
	}
	return false;
}

bool lmcSignature::computeDigest(const QByteArray& file, const QByteArray& hashedData,
								 int hashAlgo, unsigned char* digest, unsigned int* pDigestLen) {
	QByteArray input = file;
	input.append(hashedData);
	int hl = hashedData.size();
	input.append((char)0x04);
	input.append((char)0xff);
	input.append((char)((hl >> 24) & 0xff));
	input.append((char)((hl >> 16) & 0xff));
	input.append((char)((hl >> 8) & 0xff));
	input.append((char)(hl & 0xff));

	const unsigned char* d = (const unsigned char*)input.constData();
	if(hashAlgo == 8) {			//	SHA-256
		SHA256(d, input.size(), digest);
		*pDigestLen = 32;
	} else if(hashAlgo == 10) {	//	SHA-512
		SHA512(d, input.size(), digest);
		*pDigestLen = 64;
	} else if(hashAlgo == 2) {	//	SHA-1
		SHA1(d, input.size(), digest);
		*pDigestLen = 20;
	} else {
		return false;
	}
	return true;
}

QString lmcSignature::keyFilePath(void) {
	return QCoreApplication::applicationDirPath() + "/" + IDA_KEYFILE;
}

QString lmcSignature::signatureFilePath(void) {
	return QCoreApplication::applicationDirPath() + "/" + IDA_SIGFILE;
}

QByteArray lmcSignature::readFile(const QString& szPath) {
	QFile file(szPath);
	if(!file.open(QIODevice::ReadOnly))
		return QByteArray();
	QByteArray data = file.readAll();
	file.close();
	return data;
}

bool lmcSignature::hasExpectedKey(void) {
	return strlen(IDA_PUBKEY) == 64;
}

SignatureStatus lmcSignature::verify(void) {
	QString keyPath = keyFilePath();
	QString sigPath = signatureFilePath();
	if(!QFile::exists(keyPath) || !QFile::exists(sigPath))
		return SignatureFilesMissing;

	QByteArray keyData = readFile(keyPath);
	if(keyData.isEmpty())
		return SignatureFilesMissing;

	unsigned char rawKey[32];
	if(!parsePublicKey(dearmor(keyData), rawKey))
		return SignatureKeyMismatch;

	unsigned char expected[32];
	const char* hex = IDA_PUBKEY;
	for(int i = 0; i < 32; i++) {
		char byte[3] = { hex[i * 2], hex[i * 2 + 1], 0 };
		expected[i] = (unsigned char)strtol(byte, NULL, 16);
	}
	if(memcmp(rawKey, expected, 32) != 0)
		return SignatureKeyMismatch;

	QByteArray sigData = readFile(sigPath);
	unsigned char sig[64];
	int hashAlgo = 0;
	QByteArray hashedData;
	if(!parseSignature(dearmor(sigData), sig, &hashAlgo, hashedData))
		return SignatureInvalid;

	QByteArray file = readFile(QCoreApplication::applicationFilePath());
	if(file.isEmpty())
		return SignatureInvalid;

	unsigned char digest[64];
	unsigned int digestLen = 0;
	if(!computeDigest(file, hashedData, hashAlgo, digest, &digestLen))
		return SignatureInvalid;

	EVP_PKEY* pKey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, rawKey, 32);
	if(pKey == NULL)
		return SignatureInvalid;
	EVP_MD_CTX* pCtx = EVP_MD_CTX_new();
	if(pCtx == NULL) {
		EVP_PKEY_free(pKey);
		return SignatureInvalid;
	}
	EVP_DigestVerifyInit(pCtx, NULL, NULL, NULL, pKey);
	int result = EVP_DigestVerify(pCtx, sig, 64, digest, digestLen);
	EVP_MD_CTX_free(pCtx);
	EVP_PKEY_free(pKey);

	return (result == 1) ? SignatureValid : SignatureInvalid;
}
