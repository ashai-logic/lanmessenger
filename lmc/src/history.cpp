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


#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include "history.h"
#include "settings.h"
#include "stdlocation.h"

#define HC_CONNECTION			"lmc_history"
#define HC_MIGRATE_CONNECTION	"lmc_history_migrate"

static HistoryStore* g_historyStore = 0;

static QString stripDocument(const QString& html) {
	int begin = html.indexOf("<body");
	if(begin >= 0) {
		int gt = html.indexOf('>', begin);
		begin = (gt >= 0) ? gt + 1 : -1;
	}
	int end = html.lastIndexOf("</body>");
	if(begin >= 0 && end > begin)
		return html.mid(begin, end - begin);
	return html;
}

HistoryStore::HistoryStore(QObject* parent) : QObject(parent) {
	opened = false;
}

HistoryStore::~HistoryStore(void) {
	close();
}

HistoryStore* HistoryStore::instance(void) {
	if(!g_historyStore)
		g_historyStore = new HistoryStore();
	return g_historyStore;
}

QString HistoryStore::historyFile(void) {
	lmcSettings settings;
	bool sysPath = settings.value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool();
	QString path = QDir::toNativeSeparators(StdLocation::dataDir() + "/" HC_FILENAME);
	if(!sysPath)
		path = settings.value(IDS_HISTORYPATH, path).toString();
	return path;
}

QString HistoryStore::legacyId(const QString& name) {
	return QString(HC_LEGACY_PREFIX) + name;
}

void HistoryStore::setError(QString* error, const QString& message) {
	if(error)
		*error = message;
}

void HistoryStore::configureConnection(QSqlDatabase& database) {
	QSqlQuery query(database);
	query.exec("PRAGMA journal_mode=WAL");
	query.exec("PRAGMA synchronous=NORMAL");
	query.exec("PRAGMA busy_timeout=5000");
	query.exec("PRAGMA foreign_keys=ON");
	query.exec("PRAGMA secure_delete=ON");
}

bool HistoryStore::createSchema(QSqlDatabase& database, QString* error) {
	const char* statements[] = {
		"CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT)",
		"CREATE TABLE IF NOT EXISTS peers ("
			"peer_id TEXT PRIMARY KEY, peer_name TEXT NOT NULL, is_group INTEGER NOT NULL DEFAULT 0)",
		"CREATE TABLE IF NOT EXISTS messages ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT, peer_id TEXT NOT NULL, peer_name TEXT NOT NULL, "
			"sender TEXT NOT NULL, ts INTEGER NOT NULL, html TEXT NOT NULL, "
			"FOREIGN KEY(peer_id) REFERENCES peers(peer_id) ON DELETE CASCADE)",
		"CREATE INDEX IF NOT EXISTS idx_messages_peer_ts ON messages(peer_id, ts)"
	};

	QSqlQuery query(database);
	for(unsigned int index = 0; index < sizeof(statements) / sizeof(statements[0]); index++) {
		if(!query.exec(statements[index])) {
			setError(error, query.lastError().text());
			return false;
		}
	}

	query.exec("PRAGMA user_version=" + QString::number(HC_SCHEMA_VERSION));
	query.exec("PRAGMA application_id=0x4C4D4332");
	return true;
}

bool HistoryStore::open(QString* error) {
	if(opened)
		return true;

	dbPath = historyFile();

	QDir dir = QFileInfo(dbPath).dir();
	if(!dir.exists())
		dir.mkpath(dir.absolutePath());

	//	Detect a legacy (pre-SQLite) database and migrate it before opening
	bool legacy = false;
	if(QFile::exists(dbPath)) {
		QFile file(dbPath);
		if(file.open(QIODevice::ReadOnly)) {
			QByteArray head = file.read(16);
			file.close();
			if(!head.startsWith(QByteArray("SQLite format 3")))
				legacy = true;
		}
	}

	if(legacy && !migrateLegacy(dbPath, error))
		return false;

	db = QSqlDatabase::addDatabase("QSQLITE", HC_CONNECTION);
	db.setDatabaseName(dbPath);
	if(!db.open()) {
		setError(error, db.lastError().text());
		return false;
	}
	configureConnection(db);

	QSqlQuery query(db);
	if(query.exec("PRAGMA user_version") && query.next()) {
		int version = query.value(0).toInt();
		if(version > HC_SCHEMA_VERSION) {
			setError(error, "History database was created by a newer version");
			db.close();
			return false;
		}
	}

	if(!createSchema(db, error)) {
		db.close();
		return false;
	}

	opened = true;
	return true;
}

void HistoryStore::close(void) {
	if(!opened)
		return;

	if(db.isOpen())
		db.close();
	db = QSqlDatabase();
	QSqlDatabase::removeDatabase(HC_CONNECTION);
	opened = false;
}

bool HistoryStore::readLegacy(const QString& path, QList<LegacyRecord>& records, QString* error) {
	QFile file(path);
	if(!file.open(QIODevice::ReadOnly)) {
		setError(error, "Cannot open legacy history file");
		return false;
	}

	QDataStream stream(&file);

	QString marker;
	int version;
	qint16 headerSize;
	int count;
	qint64 first;
	qint64 last;
	stream >> marker >> version >> headerSize >> count >> first >> last;

	if(marker.compare("DB") != 0) {
		file.close();
		setError(error, "Unrecognized legacy history file");
		return false;
	}

	qint64 next = first;
	int guard = 0;
	while(next != 0 && guard++ < 1000000) {
		if(!stream.device()->seek(next))
			break;

		QString idMarker;
		qint64 nextIndex;
		qint64 offset;
		qint64 date;
		QString name;
		stream >> idMarker >> nextIndex >> offset >> date >> name;
		if(idMarker.compare("ID") != 0)
			break;

		if(!stream.device()->seek(offset))
			break;

		QString dataMarker;
		int length;
		QByteArray buffer;
		stream >> dataMarker >> length >> buffer;

		QString html = QString::fromUtf8(buffer.constData(), buffer.size());
		records.append(LegacyRecord(name, date, stripDocument(html)));
		next = nextIndex;
	}

	file.close();
	return true;
}

bool HistoryStore::migrateLegacy(const QString& path, QString* error) {
	QList<LegacyRecord> records;
	if(!readLegacy(path, records, error))
		return false;

	QString tempPath = path + ".new";
	QFile::remove(tempPath);

	{
		QSqlDatabase migrateDb = QSqlDatabase::addDatabase("QSQLITE", HC_MIGRATE_CONNECTION);
		migrateDb.setDatabaseName(tempPath);
		if(!migrateDb.open()) {
			setError(error, migrateDb.lastError().text());
			return false;
		}

		configureConnection(migrateDb);
		if(!createSchema(migrateDb, error)) {
			migrateDb.close();
			return false;
		}

		migrateDb.transaction();
		QSqlQuery query(migrateDb);
		for(int index = 0; index < records.count(); index++) {
			const LegacyRecord& record = records.at(index);
			QString peerId = legacyId(record.name);

			query.prepare("INSERT OR IGNORE INTO peers(peer_id, peer_name, is_group) VALUES(?, ?, 0)");
			query.addBindValue(peerId);
			query.addBindValue(record.name);
			query.exec();

			query.prepare("INSERT INTO messages(peer_id, peer_name, sender, ts, html) VALUES(?, ?, ?, ?, ?)");
			query.addBindValue(peerId);
			query.addBindValue(record.name);
			query.addBindValue(record.name);
			query.addBindValue(record.date);
			query.addBindValue(record.html);
			query.exec();
		}
		migrateDb.commit();
		migrateDb.close();
	}
	QSqlDatabase::removeDatabase(HC_MIGRATE_CONNECTION);

	//	Keep the legacy file as a backup before replacing it
	QString backupPath = path + ".v1.bak";
	QFile::remove(backupPath);
	if(!QFile::rename(path, backupPath)) {
		QFile::remove(tempPath);
		setError(error, "Cannot back up the legacy history file");
		return false;
	}
	if(!QFile::rename(tempPath, path)) {
		QFile::rename(backupPath, path);
		setError(error, "Cannot create the history database");
		return false;
	}

	return true;
}

QList<PeerInfo> HistoryStore::peers(void) {
	QList<PeerInfo> list;
	if(!opened)
		return list;

	QSqlQuery query(db);
	if(query.exec("SELECT m.peer_id, m.peer_name, COALESCE(p.is_group, 0), MAX(m.ts) AS last_ts "
			"FROM messages m LEFT JOIN peers p ON p.peer_id = m.peer_id "
			"GROUP BY m.peer_id ORDER BY last_ts DESC")) {
		while(query.next()) {
			PeerInfo info;
			info.id = query.value(0).toString();
			info.name = query.value(1).toString();
			info.isGroup = query.value(2).toInt() != 0;
			info.lastTs = query.value(3).toLongLong();
			list.append(info);
		}
	}
	return list;
}

QList<HistoryMessage> HistoryStore::messages(const QString& peerId, const QString& peerName) {
	QList<HistoryMessage> list;
	if(!opened)
		return list;

	QString legacy = peerName.isEmpty() ? QString::null : legacyId(peerName);

	QSqlQuery query(db);
	if(legacy.isNull())
		query.prepare("SELECT peer_id, peer_name, sender, ts, html FROM messages "
			"WHERE peer_id = ? ORDER BY ts ASC, id ASC");
	else
		query.prepare("SELECT peer_id, peer_name, sender, ts, html FROM messages "
			"WHERE peer_id = ? OR peer_id = ? ORDER BY ts ASC, id ASC");
	query.addBindValue(peerId);
	if(!legacy.isNull())
		query.addBindValue(legacy);

	if(query.exec()) {
		while(query.next()) {
			HistoryMessage message;
			message.peerId = query.value(0).toString();
			message.peerName = query.value(1).toString();
			message.sender = query.value(2).toString();
			message.timestamp = query.value(3).toLongLong();
			message.html = query.value(4).toString();
			list.append(message);
		}
	}
	return list;
}

bool HistoryStore::append(const QString& peerId, const QString& peerName, bool isGroup,
						  const QString& sender, qint64 timestamp, const QString& html) {
	if(!opened || peerId.isEmpty() || html.isEmpty())
		return false;

	db.transaction();
	QSqlQuery query(db);

	query.prepare("INSERT OR IGNORE INTO peers(peer_id, peer_name, is_group) VALUES(?, ?, ?)");
	query.addBindValue(peerId);
	query.addBindValue(peerName);
	query.addBindValue(isGroup ? 1 : 0);
	query.exec();

	query.prepare("UPDATE peers SET peer_name = ?, is_group = ? WHERE peer_id = ?");
	query.addBindValue(peerName);
	query.addBindValue(isGroup ? 1 : 0);
	query.addBindValue(peerId);
	query.exec();

	//	Merge any legacy (name-keyed) records of this peer into the real peer id
	if(!peerName.isEmpty()) {
		QString legacy = legacyId(peerName);
		query.prepare("UPDATE messages SET peer_id = ?, peer_name = ? WHERE peer_id = ?");
		query.addBindValue(peerId);
		query.addBindValue(peerName);
		query.addBindValue(legacy);
		query.exec();

		query.prepare("DELETE FROM peers WHERE peer_id = ?");
		query.addBindValue(legacy);
		query.exec();
	}

	//	Idempotency guard: never store an exact duplicate of a message that is
	//	already recorded for this peer. This keeps the history consistent even
	//	if a caller tries to append the same message twice (same sender,
	//	timestamp and content).
	query.prepare("SELECT 1 FROM messages WHERE peer_id = ? AND ts = ? AND sender = ? AND html = ? LIMIT 1");
	query.addBindValue(peerId);
	query.addBindValue(timestamp);
	query.addBindValue(sender);
	query.addBindValue(html);
	if(query.exec() && query.next()) {
		bool committed = db.commit();
		if(!committed) {
			db.rollback();
			return false;
		}
		return true;
	}

	query.prepare("INSERT INTO messages(peer_id, peer_name, sender, ts, html) VALUES(?, ?, ?, ?, ?)");
	query.addBindValue(peerId);
	query.addBindValue(peerName);
	query.addBindValue(sender);
	query.addBindValue(timestamp);
	query.addBindValue(html);
	bool executed = query.exec();
	bool committed = db.commit();
	if(!executed || !committed) {
		db.rollback();
		return false;
	}

	emit changed(peerId);
	return true;
}

bool HistoryStore::clearPeer(const QString& peerId, const QString& peerName) {
	if(!opened)
		return false;

	QString legacy = peerName.isEmpty() ? QString::null : legacyId(peerName);

	db.transaction();
	QSqlQuery query(db);
	if(legacy.isNull()) {
		query.prepare("DELETE FROM messages WHERE peer_id = ?");
		query.addBindValue(peerId);
		query.exec();
		query.prepare("DELETE FROM peers WHERE peer_id = ?");
		query.addBindValue(peerId);
		query.exec();
	} else {
		query.prepare("DELETE FROM messages WHERE peer_id = ? OR peer_id = ?");
		query.addBindValue(peerId);
		query.addBindValue(legacy);
		query.exec();
		query.prepare("DELETE FROM peers WHERE peer_id = ? OR peer_id = ?");
		query.addBindValue(peerId);
		query.addBindValue(legacy);
		query.exec();
	}
	bool committed = db.commit();

	QSqlQuery checkpoint(db);
	checkpoint.exec("PRAGMA wal_checkpoint(TRUNCATE)");

	emit cleared(peerId);
	return committed;
}

bool HistoryStore::clearAll(void) {
	if(!opened)
		return false;

	QSqlQuery query(db);
	query.exec("DELETE FROM messages");
	query.exec("DELETE FROM peers");
	query.exec("PRAGMA wal_checkpoint(TRUNCATE)");
	query.exec("VACUUM");

	emit cleared(QString());
	return true;
}
