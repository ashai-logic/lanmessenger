/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
**
** Contact:  qualiatech@gmail.com
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
****************************************************************************/


#ifndef HISTORY_H
#define HISTORY_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QSqlDatabase>

#define HC_FILENAME			"messenger.db"
#define HC_SCHEMA_VERSION	2
#define HC_LEGACY_PREFIX	"legacy:"

//	Used only while migrating a legacy (pre-SQLite) history file
struct LegacyRecord {
	QString name;
	qint64 date;
	QString html;

	LegacyRecord() : date(0) {}
	LegacyRecord(const QString& n, qint64 d, const QString& h) : name(n), date(d), html(h) {}
};

//	Describes a conversation peer stored in the history database
struct PeerInfo {
	QString id;
	QString name;
	bool isGroup;
	qint64 lastTs;

	PeerInfo() : isGroup(false), lastTs(0) {}
};

//	Describes a single history message
struct HistoryMessage {
	QString peerId;
	QString peerName;
	QString sender;
	qint64 timestamp;
	QString html;

	HistoryMessage() : timestamp(0) {}
};

//	Single authoritative store of message history.
//	The SQLite database is the one and only source of truth; every append is
//	written through immediately. Open chat windows keep a live in-memory view
//	that is cleared together with the store.
class HistoryStore : public QObject {
	Q_OBJECT

public:
	explicit HistoryStore(QObject* parent = 0);
	~HistoryStore(void);

	static HistoryStore* instance(void);
	static QString historyFile(void);
	static QString legacyId(const QString& name);

	bool open(QString* error = 0);
	void close(void);
	bool isOpen(void) const { return opened; }

	QList<PeerInfo> peers(void);
	QList<HistoryMessage> messages(const QString& peerId, const QString& peerName = QString());
	bool append(const QString& peerId, const QString& peerName, bool isGroup,
				const QString& sender, qint64 timestamp, const QString& html);
	bool clearPeer(const QString& peerId, const QString& peerName = QString());
	bool clearAll(void);

signals:
	void changed(const QString& peerId);
	void cleared(const QString& peerId);

private:
	bool migrateLegacy(const QString& path, QString* error);
	bool readLegacy(const QString& path, QList<LegacyRecord>& records, QString* error);
	bool createSchema(QSqlDatabase& db, QString* error);
	void configureConnection(QSqlDatabase& db);
	void setError(QString* error, const QString& message);

	QSqlDatabase db;
	bool opened;
	QString dbPath;
};

#endif // HISTORY_H
