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


#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <QString>

//	Result of checking the authorship and integrity of the application.
enum SignatureStatus {
	SignatureValid = 0,			//	lmc-ashai.asc and lmc.exe.sig match the expected key and lmc.exe
	SignatureFilesMissing,		//	lmc-ashai.asc and/or lmc.exe.sig are not present
	SignatureKeyMismatch,		//	the public key does not match the expected one
	SignatureInvalid			//	the signature check against lmc.exe failed
};

class lmcSignature {
public:
	//	Verifies the running executable against lmc-ashai.asc and lmc.exe.sig located next to it.
	static SignatureStatus verify(void);
	//	Returns whether an expected public key is embedded in the application.
	static bool hasExpectedKey(void);

private:
	static QString keyFilePath(void);
	static QString signatureFilePath(void);
	static QByteArray readFile(const QString& szPath);
	static QByteArray dearmor(const QByteArray& data);
	static QByteArray base64Decode(const QByteArray& data);
	static bool parsePublicKey(const QByteArray& keyData, unsigned char* rawKey);
	static bool parseSignature(const QByteArray& sigData, unsigned char* sig, int* pHashAlgo,
							   QByteArray& hashedData);
	static bool computeDigest(const QByteArray& file, const QByteArray& hashedData,
							  int hashAlgo, unsigned char* digest, unsigned int* pDigestLen);
};

#endif // SIGNATURE_H
