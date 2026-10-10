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
#include <QFile>
#include "aboutdialog.h"

//	constructor
lmcAboutDialog::lmcAboutDialog(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags) {
	ui.setupUi(this);
	//	set minimum size
	layout()->setSizeConstraint(QLayout::SetMinimumSize);
	//	remove the help button from window button group
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);
}

lmcAboutDialog::~lmcAboutDialog(void) {
}

void lmcAboutDialog::init(void) {
	setWindowIcon(QIcon(IDR_APPICON));

	pSettings = new lmcSettings();
	//	Check the authorship and integrity of the application on every opening
	signatureStatus = lmcSignature::verify();
	setUIText();

	ui.tabWidget->setCurrentIndex(0);
}

void lmcAboutDialog::settingsChanged(void) {
}

void lmcAboutDialog::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QDialog::changeEvent(pEvent);
}

void lmcAboutDialog::setUIText(void) {
	ui.retranslateUi(this);

	QString title = tr("About %1");
	setWindowTitle(title.arg(lmcStrings::appName()));

	//	The fork brand is shown only here, in the About dialog.
	ui.lblTitle->setText(lmcStrings::appName() + "\n" IDA_VERSION " " IDA_BRAND);
	ui.lblLogoSmall->setPixmap(QPixmap(IDR_LOGOSMALL));
#if defined(QT_NO_DEBUG)
#define DEBUGINFO " "
#else
#define DEBUGINFO " debug"
#endif
    ui.lblQtVersion->setText(QString("Qt %1  %2" DEBUGINFO "\n%3 %4")
                             .arg(QT_VERSION_STR,
                                  QSysInfo::buildCpuArchitecture(),
                                  QSysInfo::productType(),
                                  QSysInfo::productVersion()));

	//	Build rich text so that only the verification result is highlighted.
	QString description(QString(lmcStrings::appDesc()).toHtmlEscaped());
	description.replace("\n", "<br>");
	description.append("<br><br>" + QString(IDA_COPYRIGHT).toHtmlEscaped());
	description.append("<br><br>" +
		tr("This is an unofficial fork of LAN Messenger and is not affiliated with or "
		   "endorsed by Qualia Digital Solutions.").toHtmlEscaped());
	description.append("<br>" + signatureText());
	description.append("<br><br>");
	description.append(tr("Source code:") + " " + QString(IDA_DOMAIN).toHtmlEscaped());
	description.append("<br>" + tr("Contact:") + " " + QString(IDA_CONTACT).toHtmlEscaped());
	ui.lblDescription->setText(description);

	QFile thanks(IDR_THANKSTEXT);
	if(thanks.open(QIODevice::ReadOnly)) {
		ui.txtThanks->setPlainText(QString(thanks.readAll().constData()));
		thanks.close();
	}

	QFile license(IDR_LICENSETEXT);
	if(license.open(QIODevice::ReadOnly)) {
		ui.txtLicense->setPlainText(QString(license.readAll().constData()));
		license.close();
	}
}

QString lmcAboutDialog::signatureText(void) {
	bool valid = (signatureStatus == SignatureValid);
	QString color = valid ? "#008000" : "#cc0000";
	QString status = valid ? tr("OK") : tr("FAILED");
	//	Only the result itself is colored and bold, not the whole line.
	return tr("Authorship and integrity verification:") +
		" <span style=\"color:" + color + "; font-weight:bold;\">" + status + "</span>";
}
