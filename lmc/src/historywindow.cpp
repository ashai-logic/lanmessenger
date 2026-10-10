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


#include <QDesktopWidget>
#include <QTextDocument>
#include "historywindow.h"

//	Returns the plain text of a stored history entry, used to match it against
//	the search string without being confused by the HTML markup.
static QString historyPlainText(const QString& html) {
	QTextDocument document;
	document.setHtml(html);
	return document.toPlainText();
}

lmcHistoryWindow::lmcHistoryWindow(QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags) {
	ui.setupUi(this);

	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);

	pMessageLog = new lmcMessageLog(ui.fraMessageLog);
	ui.logLayout->addWidget(pMessageLog);
	pMessageLog->setAcceptDrops(false);

	QList<int> sizes;
	sizes.append(width() * 0.35);
	sizes.append(width() - width() * 0.35 - ui.splitter->handleWidth());
	ui.splitter->setSizes(sizes);
	QRect scr = QApplication::desktop()->screenGeometry();
	move(scr.center() - rect().center());

	connect(ui.tvMsgList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
		this, SLOT(tvMsgList_currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(ui.btnClearHistory, SIGNAL(clicked()), this, SLOT(btnClearHistory_clicked()));
	connect(ui.txtSearch, SIGNAL(textChanged(QString)), this, SLOT(txtSearch_textChanged(QString)));

    ui.tvMsgList->installEventFilter(this);
    pMessageLog->installEventFilter(this);
    ui.btnClearHistory->installEventFilter(this);
    ui.btnClose->installEventFilter(this);
}

lmcHistoryWindow::~lmcHistoryWindow() {
}

void lmcHistoryWindow::init(QString* lpszUserId, QString* lpszUserName) {
	setWindowIcon(QIcon(IDR_APPICON));
    ui.splitter->setStyleSheet("QSplitter::handle { image: url(" IDR_HGRIP "); }");

	pMessageLog->setAutoScroll(false);

	filterId = lpszUserId ? *lpszUserId : QString::null;
	filterName = lpszUserName ? *lpszUserName : QString::null;

	pSettings = new lmcSettings();
	restoreGeometry(pSettings->value(IDS_WINDOWHISTORY).toByteArray());
	ui.splitter->restoreState(pSettings->value(IDS_SPLITTERHISTORY).toByteArray());
	setUIText();

	displayList();
}

void lmcHistoryWindow::setFilter(QString* lpszUserId, QString* lpszUserName) {
	filterId = lpszUserId ? *lpszUserId : QString::null;
	filterName = lpszUserName ? *lpszUserName : QString::null;
	setUIText();
	displayList();
}

void lmcHistoryWindow::updateList(void) {
	displayList();
}

void lmcHistoryWindow::stop(void) {
	pSettings->setValue(IDS_WINDOWHISTORY, saveGeometry());
	pSettings->setValue(IDS_SPLITTERHISTORY, ui.splitter->saveState());
}

void lmcHistoryWindow::settingsChanged(void) {
}

bool lmcHistoryWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
    Q_UNUSED(pObject);
    if(pEvent->type() == QEvent::KeyPress) {
        QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
        if(pKeyEvent->key() == Qt::Key_Escape) {
            close();
            return true;
        }
    }

    return false;
}

void lmcHistoryWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcHistoryWindow::tvMsgList_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(previous);

	if(current) {
		activeId = current->data(0, DataRole).toString();
		activeName = current->text(0);
	} else {
		activeId = QString::null;
		activeName = QString::null;
	}

	refreshView();
}

void lmcHistoryWindow::txtSearch_textChanged(const QString& text) {
	searchText = text.trimmed();
	refreshView();
}

void lmcHistoryWindow::btnClearHistory_clicked(void) {
	HistoryStore* store = HistoryStore::instance();
	if(filterId.isNull() && filterName.isNull())
		store->clearAll();
	else
		store->clearPeer(filterId.isNull() ? QString::null : filterId,
						filterName.isNull() ? QString::null : filterName);
	displayList();
}

void lmcHistoryWindow::setUIText(void) {
	ui.retranslateUi(this);

	QString title = tr("Message History");
	if(!filterName.isNull() && !filterName.isEmpty())
		title = title + " - " + filterName;
	setWindowTitle(title);
}

void lmcHistoryWindow::displayList(void) {
	ui.tvMsgList->clear();
	peerList.clear();

	HistoryStore* store = HistoryStore::instance();

	if(!filterId.isNull() || (!filterName.isNull() && !filterName.isEmpty())) {
		PeerInfo info;
		info.id = filterId.isNull() ? QString::null : filterId;
		info.name = filterName.isNull() ? QString::null : filterName;
		info.isGroup = false;
		info.lastTs = 0;
		peerList.append(info);
	} else {
		peerList = store->peers();
	}

	for(int index = 0; index < peerList.count(); index++) {
		lmcHistoryTreeWidgetItem* pItem = new lmcHistoryTreeWidgetItem();
		pItem->setText(0, peerList[index].name);
		pItem->setData(0, DataRole, peerList[index].id);
		if(peerList[index].lastTs > 0) {
			QDateTime date = QDateTime::fromMSecsSinceEpoch(peerList[index].lastTs);
			pItem->setText(1, date.toString(Qt::SystemLocaleDate));
			pItem->setData(1, DataRole, date);
		}
		pItem->setSizeHint(0, QSize(0, 20));
		ui.tvMsgList->addTopLevelItem(pItem);
	}

	ui.tvMsgList->sortByColumn(1, Qt::DescendingOrder);

	//	Rebuilding the list invalidates the selection, so restore it without
	//	triggering the corresponding slot twice and refresh the message pane.
	ui.tvMsgList->blockSignals(true);
	if(ui.tvMsgList->topLevelItemCount() > 0)
		ui.tvMsgList->setCurrentItem(ui.tvMsgList->topLevelItem(0));
	ui.tvMsgList->blockSignals(false);

	QTreeWidgetItem* current = ui.tvMsgList->currentItem();
	if(current) {
		activeId = current->data(0, DataRole).toString();
		activeName = current->text(0);
	} else {
		activeId = QString::null;
		activeName = QString::null;
	}

	refreshView();
}

bool lmcHistoryWindow::isFiltered(void) const {
	return !filterId.isNull() || (!filterName.isNull() && !filterName.isEmpty());
}

//	Chooses what to display in the message pane depending on whether a search is
//	active. With an empty search box this is simply the selected conversation.
void lmcHistoryWindow::refreshView(void) {
	if(searchText.isEmpty())
		loadConversation(activeId, activeName);
	else
		showSearchResults();
}

//	Displays the messages that match the current search string. When the window
//	shows the history of a single contact ("conversation history") the search
//	covers only that contact; in the Tools - History window (no contact filter)
//	it covers the entire history of all contacts.
void lmcHistoryWindow::showSearchResults(void) {
	HistoryStore* store = HistoryStore::instance();

	bool filtered = isFiltered();

	QList<PeerInfo> peers;
	if(filtered) {
		PeerInfo info;
		info.id = filterId.isNull() ? QString::null : filterId;
		info.name = filterName.isNull() ? QString::null : filterName;
		peers.append(info);
	} else {
		peers = store->peers();
	}

	QString html = lmcMessageLog::historyDocumentHeader();
	int matches = 0;

	for(int p = 0; p < peers.count(); p++) {
		QList<HistoryMessage> messages = store->messages(peers.at(p).id, peers.at(p).name);
		QString groupHtml;
		for(int m = 0; m < messages.count(); m++) {
			const HistoryMessage& message = messages.at(m);
			if(!historyPlainText(message.html).contains(searchText, Qt::CaseInsensitive))
				continue;

			if(!filtered && groupHtml.isEmpty())
				groupHtml.append("<p class='searchPeer'><b>" +
					message.peerName.toHtmlEscaped() + "</b></p>");

			groupHtml.append(message.html);
			matches++;
		}
		html.append(groupHtml);
	}

	if(matches == 0)
		html.append("<p>" + tr("No messages found.") + "</p>");

	html.append(lmcMessageLog::historyDocumentFooter());
	pMessageLog->setHtml(html);
}

void lmcHistoryWindow::loadConversation(const QString& peerId, const QString& peerName) {
	if(peerId.isNull() && peerName.isNull()) {
		pMessageLog->setHtml("<html></html>");
		return;
	}

	QList<HistoryMessage> messages = HistoryStore::instance()->messages(peerId, peerName);

	QString html = lmcMessageLog::historyDocumentHeader();
	for(int index = 0; index < messages.count(); index++)
		html.append(messages.at(index).html);
	html.append(lmcMessageLog::historyDocumentFooter());

	pMessageLog->setHtml(html);
}
