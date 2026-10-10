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
** Modified by ASHAI, 2026.
**
****************************************************************************/


#include <QDesktopWidget>
#include "helpwindow.h"

lmcHelpWindow::lmcHelpWindow(QRect* pRect, QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);

	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);

	move(pRect->center() - rect().center());
	QRect screenRect = QApplication::desktop()->screenGeometry();
	if(!screenRect.contains(geometry(), true)) {
		QRect windowRect = geometry();
		if(windowRect.right() > screenRect.right())
			windowRect.translate(screenRect.right() - windowRect.right(), 0);
		if(windowRect.left() < screenRect.left())
			windowRect.translate(qAbs(windowRect.left() - screenRect.left()), 0);
		if(windowRect.bottom() > screenRect.bottom())
			windowRect.translate(0, screenRect.bottom() - windowRect.bottom());
		if(windowRect.top() < screenRect.top())
			windowRect.translate(0, qAbs(windowRect.top() - screenRect.top()));
		setGeometry(windowRect);
	}

    ui.txtHelp->installEventFilter(this);
    ui.btnClose->installEventFilter(this);
}

lmcHelpWindow::~lmcHelpWindow() {
}

void lmcHelpWindow::init(void) {
	setWindowIcon(QIcon(IDR_APPICON));

	pSettings = new lmcSettings();
	restoreGeometry(pSettings->value(IDS_WINDOWHELP).toByteArray());
	setUIText();
}

void lmcHelpWindow::stop(void) {
	pSettings->setValue(IDS_WINDOWHELP, saveGeometry());
}

void lmcHelpWindow::settingsChanged(void) {
}

bool lmcHelpWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
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

void lmcHelpWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcHelpWindow::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(tr("Help"));

	QString html = tr(
		"<html>"
			"<head>"
				"<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
			"</head>"
			"<body>"
				"<h2>Welcome to LAN Messenger Help</h2>"
			"</body>"
		"</html>");
	ui.txtHelp->setHtml(html);
}
