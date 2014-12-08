/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
  Copyright (c) 2009 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// define this to copy all html that is written to the readerwindow to
// filehtmlwriter.out in the current working directory
#include "kmreaderwin.h"

#include "settings/globalsettings.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include "kernel/mailkernel.h"
#include "dialog/addemailtoexistingcontactdialog.h"
#include "job/addemailtoexistingcontactjob.h"

#include "kdepim-version.h"
#include <KEmailAddress>
#include <libkdepim/job/addemailaddressjob.h>
#include <libkdepim/job/openemailaddressjob.h>
#include <libkdepim/job/addemaildisplayjob.h>
#include <libkdepim/misc/broadcaststatus.h>
#include "kmcommands.h"
#include "mailcommon/mdn/sendmdnhandler.h"
#include <QVBoxLayout>
#include "messageviewer/header/headerstrategy.h"
#include "messageviewer/header/headerstyle.h"
#include "messageviewer/viewer/mailwebview.h"
#include "messageviewer/utils/markmessagereadhandler.h"
#include "messageviewer/settings/globalsettings.h"
#include "messageviewer/viewer/csshelper.h"
using MessageViewer::CSSHelper;
#include "util.h"
#include "utils/stringutil.h"
#include <QCryptographicHash>
#include <kmime/kmime_mdn.h>

#include "messageviewer/viewer/viewer.h"
using namespace MessageViewer;
#include <messagecore/settings/globalsettings.h>

#include "messageviewer/viewer/attachmentstrategy.h"
#include "messagecomposer/sender/messagesender.h"
#include "messagecomposer/helper/messagefactory.h"
#include "messagecomposer/composer/composer.h"
#include "messagecomposer/part/textpart.h"
#include "messagecomposer/part/infopart.h"

#include <KIO/JobUiDelegate>
using MessageComposer::MessageFactory;

#include "messagecore/helpers/messagehelpers.h"

#include <Akonadi/Contact/ContactEditorDialog>

#include <kde_file.h>
#include <qdebug.h>
#include <KLocalizedString>
#include <QAction>
#include <kcodecs.h>
#include <ktoggleaction.h>
#include <kservice.h>
#include <KActionCollection>
#include <KMessageBox>
#include <QMenu>

#include <QClipboard>

// X headers...
#undef Never
#undef Always

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <util/mailutil.h>

using namespace KMail;
using namespace MailCommon;


KMReaderWin::KMReaderWin(QWidget *aParent,
                         QWidget *mainWindow,
                         KActionCollection *actionCollection,
                         Qt::WindowFlags aFlags)
    : QWidget(aParent, aFlags),
      mMainWindow(mainWindow),
      mActionCollection(actionCollection),
      mMailToComposeAction(0),
      mMailToReplyAction(0),
      mMailToForwardAction(0),
      mAddAddrBookAction(0),
      mOpenAddrBookAction(0),
      mUrlSaveAsAction(0),
      mAddBookmarksAction(0),
      mAddEmailToExistingContactAction(0)
{
    createActions();
    QVBoxLayout *vlay = new QVBoxLayout(this);
    vlay->setMargin(0);
    mViewer = new Viewer(this, mainWindow, mActionCollection);
    mViewer->setExternalWindow(true);
    mViewer->setAppName(QLatin1String("KMail"));
    connect(mViewer, SIGNAL(urlClicked(Akonadi::Item,KUrl)), this, SLOT(slotUrlClicked(Akonadi::Item,KUrl)));
    connect(mViewer, SIGNAL(requestConfigSync()), kmkernel, SLOT(slotRequestConfigSync()), Qt::QueuedConnection);   // happens anyway on shutdown, so we can skip it there with using a queued connection
    connect(mViewer, SIGNAL(makeResourceOnline(MessageViewer::Viewer::ResourceOnlineMode)), kmkernel, SLOT(makeResourceOnline(MessageViewer::Viewer::ResourceOnlineMode)));
    connect(mViewer, &MessageViewer::Viewer::showReader, this, &KMReaderWin::slotShowReader);
    connect(mViewer, &MessageViewer::Viewer::showMessage, this, &KMReaderWin::slotShowMessage);
    connect(mViewer, &MessageViewer::Viewer::showStatusBarMessage, this, &KMReaderWin::showStatusBarMessage);
    connect(mViewer, SIGNAL(deleteMessage(Akonadi::Item)), this, SLOT(slotDeleteMessage(Akonadi::Item)));

    mViewer->addMessageLoadedHandler(new MessageViewer::MarkMessageReadHandler(this));
    mViewer->addMessageLoadedHandler(new MailCommon::SendMdnHandler(kmkernel, this));

    vlay->addWidget(mViewer);
    readConfig();

}

void KMReaderWin::createActions()
{
    KActionCollection *ac = mActionCollection;
    if (!ac) {
        return;
    }
    //
    // Message Menu
    //
    // new message to
    mMailToComposeAction = new QAction(QIcon::fromTheme(QLatin1String("mail-message-new")),
                                       i18n("New Message To..."), this);
    ac->addAction(QLatin1String("mail_new"), mMailToComposeAction);
    ac->setShortcutsConfigurable(mMailToComposeAction, false);
    connect(mMailToComposeAction, &QAction::triggered, this, &KMReaderWin::slotMailtoCompose);

    // reply to
    mMailToReplyAction = new QAction(QIcon::fromTheme(QLatin1String("mail-reply-sender")),
                                     i18n("Reply To..."), this);
    ac->addAction(QLatin1String("mailto_reply"), mMailToReplyAction);
    ac->setShortcutsConfigurable(mMailToReplyAction, false);
    connect(mMailToReplyAction, &QAction::triggered, this, &KMReaderWin::slotMailtoReply);

    // forward to
    mMailToForwardAction = new QAction(QIcon::fromTheme(QLatin1String("mail-forward")),
                                       i18n("Forward To..."), this);
    ac->setShortcutsConfigurable(mMailToForwardAction, false);
    ac->addAction(QLatin1String("mailto_forward"), mMailToForwardAction);
    connect(mMailToForwardAction, &QAction::triggered, this, &KMReaderWin::slotMailtoForward);

    // add to addressbook
    mAddAddrBookAction = new QAction(QIcon::fromTheme(QLatin1String("contact-new")),
                                     i18n("Add to Address Book"), this);
    ac->setShortcutsConfigurable(mAddAddrBookAction, false);
    ac->addAction(QLatin1String("add_addr_book"), mAddAddrBookAction);
    connect(mAddAddrBookAction, &QAction::triggered, this, &KMReaderWin::slotMailtoAddAddrBook);

    mAddEmailToExistingContactAction = new QAction(QIcon::fromTheme(QLatin1String("contact-new")),
            i18n("Add to Existing Contact"), this);
    ac->setShortcutsConfigurable(mAddEmailToExistingContactAction, false);
    ac->addAction(QLatin1String("add_to_existing_contact"), mAddAddrBookAction);
    connect(mAddEmailToExistingContactAction, &QAction::triggered, this, &KMReaderWin::slotMailToAddToExistingContact);

    // open in addressbook
    mOpenAddrBookAction = new QAction(QIcon::fromTheme(QLatin1String("view-pim-contacts")),
                                      i18n("Open in Address Book"), this);
    ac->setShortcutsConfigurable(mOpenAddrBookAction, false);
    ac->addAction(QLatin1String("openin_addr_book"), mOpenAddrBookAction);
    connect(mOpenAddrBookAction, &QAction::triggered, this, &KMReaderWin::slotMailtoOpenAddrBook);
    // bookmark message
    mAddBookmarksAction = new QAction(QIcon::fromTheme(QLatin1String("bookmark-new")), i18n("Bookmark This Link"), this);
    ac->setShortcutsConfigurable(mAddBookmarksAction, false);
    ac->addAction(QLatin1String("add_bookmarks"), mAddBookmarksAction);
    connect(mAddBookmarksAction, &QAction::triggered, this, &KMReaderWin::slotAddBookmarks);

    mEditContactAction = new QAction(QIcon::fromTheme(QLatin1String("view-pim-contacts")),
                                     i18n("Edit contact..."), this);
    ac->setShortcutsConfigurable(mEditContactAction, false);
    ac->addAction(QLatin1String("edit_contact"), mOpenAddrBookAction);
    connect(mEditContactAction, &QAction::triggered, this, &KMReaderWin::slotEditContact);

    // save URL as
    mUrlSaveAsAction = new QAction(i18n("Save Link As..."), this);
    ac->addAction(QLatin1String("saveas_url"), mUrlSaveAsAction);
    ac->setShortcutsConfigurable(mUrlSaveAsAction, false);
    connect(mUrlSaveAsAction, &QAction::triggered, this, &KMReaderWin::slotUrlSave);

    // find text
    QAction *action = new QAction(QIcon::fromTheme(QLatin1String("edit-find")), i18n("&Find in Message..."), this);
    ac->addAction(QLatin1String("find_in_messages"), action);
    connect(action, &QAction::triggered, this, &KMReaderWin::slotFind);
    action->setShortcut(KStandardShortcut::find().first());

    // save Image On Disk
    mImageUrlSaveAsAction = new QAction(i18n("Save Image On Disk..."), this);
    ac->addAction(QLatin1String("saveas_imageurl"), mImageUrlSaveAsAction);
    ac->setShortcutsConfigurable(mImageUrlSaveAsAction, false);
    connect(mImageUrlSaveAsAction, &QAction::triggered, this, &KMReaderWin::slotSaveImageOnDisk);

    // View html options
    mViewHtmlOptions = new QMenu(i18n("Show HTML Format"));
    mViewAsHtml = new QAction(i18n("Show HTML format when mail comes from this contact"), mViewHtmlOptions);
    ac->setShortcutsConfigurable(mViewAsHtml, false);
    connect(mViewAsHtml, &QAction::triggered, this, &KMReaderWin::slotContactHtmlOptions);
    mViewAsHtml->setCheckable(true);
    mViewHtmlOptions->addAction(mViewAsHtml);

    mLoadExternalReference = new QAction(i18n("Load external reference when mail comes for this contact"), mViewHtmlOptions);
    ac->setShortcutsConfigurable(mLoadExternalReference, false);
    connect(mLoadExternalReference, &QAction::triggered, this, &KMReaderWin::slotContactHtmlOptions);
    mLoadExternalReference->setCheckable(true);
    mViewHtmlOptions->addAction(mLoadExternalReference);

    mShareImage = new QAction(i18n("Share image..."), this);
    ac->addAction(QLatin1String("share_imageurl"), mShareImage);
    ac->setShortcutsConfigurable(mShareImage, false);
    connect(mShareImage, &QAction::triggered, this, &KMReaderWin::slotShareImage);

}

void KMReaderWin::setUseFixedFont(bool useFixedFont)
{
    mViewer->setUseFixedFont(useFixedFont);
}

bool KMReaderWin::isFixedFont() const
{
    return mViewer->isFixedFont();
}


KMReaderWin::~KMReaderWin()
{
}

void KMReaderWin::readConfig(void)
{
    mViewer->readConfig();
}

void KMReaderWin::setAttachmentStrategy(const AttachmentStrategy *strategy)
{
    mViewer->setAttachmentStrategy(strategy);
}

void KMReaderWin::setHeaderStyleAndStrategy(HeaderStyle *style,
        HeaderStrategy *strategy)
{
    mViewer->setHeaderStyleAndStrategy(style, strategy);
}
void KMReaderWin::setOverrideEncoding(const QString &encoding)
{
    mViewer->setOverrideEncoding(encoding);
}


void KMReaderWin::clearCache()
{
    clear();
}

// enter items for the "Important changes" list here:
static const char *const kmailChanges[] = {
    I18N_NOOP("KMail is now based on the Akonadi Personal Information Management framework, which brings many "
    "changes all around.")
};
static const int numKMailChanges =
    sizeof kmailChanges / sizeof * kmailChanges;

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char *const kmailNewFeatures[] = {
    I18N_NOOP("Push email (IMAP IDLE)"),
    I18N_NOOP("Improved virtual folders"),
    I18N_NOOP("Improved searches"),
    I18N_NOOP("Support for adding notes (annotations) to mails"),
    I18N_NOOP("Tag folders"),
    I18N_NOOP("Less GUI freezes, mail checks happen in the background")
};
static const int numKMailNewFeatures =
    sizeof kmailNewFeatures / sizeof * kmailNewFeatures;


//static
QString KMReaderWin::newFeaturesMD5()
{
    QByteArray str;
    for (int i = 0 ; i < numKMailChanges ; ++i) {
        str += kmailChanges[i];
    }
    for (int i = 0 ; i < numKMailNewFeatures ; ++i) {
        str += kmailNewFeatures[i];
    }
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(str);
    return QLatin1String(md5.result().toBase64());
}

void KMReaderWin::displaySplashPage(const QString &info)
{
    mViewer->displaySplashPage(info);
}

void KMReaderWin::displayBusyPage()
{
    const QString info =
        i18n("<h2 style='margin-top: 0px;'>Retrieving Folder Contents</h2><p>Please wait . . .</p>&nbsp;");

    displaySplashPage(info);
}

void KMReaderWin::displayOfflinePage()
{
    const QString info =
        i18n("<h2 style='margin-top: 0px;'>Offline</h2><p>KMail is currently in offline mode. "
             "Click <a href=\"kmail:goOnline\">here</a> to go online . . .</p>&nbsp;");

    displaySplashPage(info);
}

void KMReaderWin::displayResourceOfflinePage()
{
    const QString info =
        i18n("<h2 style='margin-top: 0px;'>Offline</h2><p>Account is currently in offline mode. "
             "Click <a href=\"kmail:goResourceOnline\">here</a> to go online . . .</p>&nbsp;");

    displaySplashPage(info);
}

void KMReaderWin::displayAboutPage()
{
    KLocalizedString info =
        ki18nc("%1: KMail version; %2: help:// URL; "
               "%3: generated list of new features; "
               "%4: First-time user text (only shown on first start); "
               "%5: generated list of important changes; "
               "--- end of comment ---",
               "<h2 style='margin-top: 0px;'>Welcome to KMail %1</h2><p>KMail is the email client by KDE. "
               "It is designed to be fully compatible with "
               "Internet mailing standards including MIME, SMTP, POP3, and IMAP."
               "</p>\n"
               "<ul><li>KMail has many powerful features which are described in the "
               "<a href=\"%2\">documentation</a></li>\n"
               "%5\n" // important changes
               "%3\n" // new features
               "%4\n" // first start info
               "<p>We hope that you will enjoy KMail.</p>\n"
               "<p>Thank you,</p>\n"
               "<p style='margin-bottom: 0px'>&nbsp; &nbsp; The KMail Team</p>")
        .subs(QLatin1String(KDEPIM_VERSION))
        .subs(QLatin1String("help:/kmail/index.html"));

    if ((numKMailNewFeatures > 1) || (numKMailNewFeatures == 1 && strlen(kmailNewFeatures[0]) > 0)) {
        QString featuresText =
            i18n("<p>Some of the new features in this release of KMail include "
                 "(compared to KMail %1, which is part of KDE Software Compilation %2):</p>\n",
                 QLatin1String("1.13"), QLatin1String(KDEPIM_VERSION));  // prior KMail and KDE version
        featuresText += QLatin1String("<ul>\n");
        for (int i = 0 ; i < numKMailNewFeatures ; ++i) {
            featuresText += QLatin1String("<li>") + i18n(kmailNewFeatures[i]) + QLatin1String("</li>\n");
        }
        featuresText += QLatin1String("</ul>\n");
        info = info.subs(featuresText);
    } else {
        info = info.subs(QString());    // remove the place holder
    }

    if (kmkernel->firstStart()) {
        info = info.subs(i18n("<p>Please take a moment to fill in the KMail "
                              "configuration panel at Settings-&gt;Configure "
                              "KMail.\n"
                              "You need to create at least a default identity and "
                              "an incoming as well as outgoing mail account."
                              "</p>\n"));
    } else {
        info = info.subs(QString());   // remove the place holder
    }

    if ((numKMailChanges > 1) || (numKMailChanges == 1 && strlen(kmailChanges[0]) > 0)) {
        QString changesText =
            i18n("<p><span style='font-size:125%; font-weight:bold;'>"
                 "Important changes</span> (compared to KMail %1):</p>\n",
                 QLatin1String("1.13"));
        changesText += QLatin1String("<ul>\n");
        for (int i = 0 ; i < numKMailChanges ; ++i) {
            changesText += i18n("<li>%1</li>\n", i18n(kmailChanges[i]));
        }
        changesText += QLatin1String("</ul>\n");
        info = info.subs(changesText);
    } else {
        info = info.subs(QString());    // remove the place holder
    }

    displaySplashPage(info.toString());
}


void KMReaderWin::slotFind()
{
    mViewer->slotFind();
}

void KMReaderWin::slotCopySelectedText()
{
    QString selection = mViewer->selectedText();
    selection.replace(QChar::Nbsp, QLatin1Char(' '));
    QApplication::clipboard()->setText(selection);
}

void KMReaderWin::setMsgPart(KMime::Content *aMsgPart)
{
    mViewer->setMessagePart(aMsgPart);
}


QString KMReaderWin::copyText() const
{
    return mViewer->selectedText();
}


MessageViewer::Viewer::DisplayFormatMessage KMReaderWin::displayFormatMessageOverwrite() const
{
    return mViewer->displayFormatMessageOverwrite();
}

void KMReaderWin::setDisplayFormatMessageOverwrite(MessageViewer::Viewer::DisplayFormatMessage format)
{
    mViewer->setDisplayFormatMessageOverwrite(format);
}

void KMReaderWin::setHtmlLoadExtOverride(bool override)
{
    mViewer->setHtmlLoadExtOverride(override);
}


bool KMReaderWin::htmlMail() const
{
    return mViewer->htmlMail();
}


bool KMReaderWin::htmlLoadExternal()
{
    return mViewer->htmlLoadExternal();
}


Akonadi::Item KMReaderWin::message() const
{
    return mViewer->messageItem();
}


void KMReaderWin::slotMailtoCompose()
{
    KMCommand *command = new KMMailtoComposeCommand(urlClicked(), message());
    command->start();
}


void KMReaderWin::slotMailtoForward()
{
    KMCommand *command = new KMMailtoForwardCommand(mMainWindow, urlClicked(),
            message());
    command->start();
}


void KMReaderWin::slotMailtoAddAddrBook()
{
    const KUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url);

    KPIM::AddEmailAddressJob *job = new KPIM::AddEmailAddressJob(emailString, mMainWindow, this);
    job->start();
}

void KMReaderWin::slotMailToAddToExistingContact()
{
    const KUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url);
    QPointer<AddEmailToExistingContactDialog> dlg = new AddEmailToExistingContactDialog(this);
    if (dlg->exec()) {
        Akonadi::Item item = dlg->selectedContact();
        if (item.isValid()) {
            AddEmailToExistingContactJob *job = new AddEmailToExistingContactJob(item, emailString, this);
            job->start();
        }
    }
    delete dlg;
}


void KMReaderWin::slotMailtoOpenAddrBook()
{
    const KUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url).toLower();

    KPIM::OpenEmailAddressJob *job = new KPIM::OpenEmailAddressJob(emailString, mMainWindow, this);
    job->start();
}


void KMReaderWin::slotAddBookmarks()
{
    const KUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    KMCommand *command = new KMAddBookmarksCommand(url, this);
    command->start();
}


void KMReaderWin::slotUrlSave()
{
    const KUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    KMCommand *command = new KMUrlSaveCommand(url, mMainWindow);
    command->start();
}

void KMReaderWin::slotSaveImageOnDisk()
{
    const KUrl url = imageUrlClicked();
    if (url.isEmpty()) {
        return;
    }
    KMCommand *command = new KMUrlSaveCommand(url, mMainWindow);
    command->start();
}


void KMReaderWin::slotMailtoReply()
{
    KMCommand *command = new KMMailtoReplyCommand(mMainWindow, urlClicked(),
            message(), copyText());
    command->start();
}

CSSHelper *KMReaderWin::cssHelper() const
{
    return mViewer->cssHelper();
}

bool KMReaderWin::htmlLoadExtOverride() const
{
    return mViewer->htmlLoadExtOverride();
}
void KMReaderWin::setDecryptMessageOverwrite(bool overwrite)
{
    mViewer->setDecryptMessageOverwrite(overwrite);
}
const AttachmentStrategy *KMReaderWin::attachmentStrategy() const
{
    return mViewer->attachmentStrategy();
}

QString KMReaderWin::overrideEncoding() const
{
    return mViewer->overrideEncoding();
}

KToggleAction *KMReaderWin::toggleFixFontAction() const
{
    return mViewer->toggleFixFontAction();
}

QAction *KMReaderWin::toggleMimePartTreeAction() const
{
    return mViewer->toggleMimePartTreeAction();
}

QAction *KMReaderWin::selectAllAction() const
{
    return mViewer->selectAllAction();
}

const HeaderStrategy *KMReaderWin::headerStrategy() const
{
    return mViewer->headerStrategy();
}

HeaderStyle *KMReaderWin::headerStyle() const
{
    return mViewer->headerStyle();
}

QAction *KMReaderWin::copyURLAction() const
{
    return mViewer->copyURLAction();
}

QAction *KMReaderWin::copyImageLocation() const
{
    return mViewer->copyImageLocation();
}

QAction *KMReaderWin::copyAction() const
{
    return mViewer->copyAction();
}

QAction *KMReaderWin::viewSourceAction() const
{
    return mViewer->viewSourceAction();
}

QAction *KMReaderWin::saveAsAction() const
{
    return mViewer->saveAsAction();
}

QAction *KMReaderWin::findInMessageAction() const
{
    return mViewer->findInMessageAction();
}

QAction *KMReaderWin::urlOpenAction() const
{
    return mViewer->urlOpenAction();
}
void KMReaderWin::setPrinting(bool enable)
{
    mViewer->setPrinting(enable);
}

QAction *KMReaderWin::speakTextAction() const
{
    return mViewer->speakTextAction();
}

QAction *KMReaderWin::downloadImageToDiskAction() const
{
    return mImageUrlSaveAsAction;
}

QAction *KMReaderWin::translateAction() const
{
    return mViewer->translateAction();
}

void KMReaderWin::clear(bool force)
{
    mViewer->clear(force ? Viewer::Force : Viewer::Delayed);
}

void KMReaderWin::setMessage(const Akonadi::Item &item, Viewer::UpdateMode updateMode)
{
    qDebug() << Q_FUNC_INFO << parentWidget();
    mViewer->setMessageItem(item, updateMode);
}

void KMReaderWin::setMessage(KMime::Message::Ptr message)
{
    mViewer->setMessage(message);
}

KUrl KMReaderWin::urlClicked() const
{
    return mViewer->urlClicked();
}

KUrl KMReaderWin::imageUrlClicked() const
{
    return mViewer->imageUrlClicked();
}

void KMReaderWin::update(bool force)
{
    mViewer->update(force ? Viewer::Force : Viewer::Delayed);
}

void KMReaderWin::slotUrlClicked(const Akonadi::Item &item, const KUrl &url)
{
    if (item.isValid() && item.parentCollection().isValid()) {
        QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(
                MailCommon::Util::updatedCollection(item.parentCollection()), false);
        KMail::Util::handleClickedURL(url, fd);
        return;
    }
    //No folder so we can't have identity and template.
    KMail::Util::handleClickedURL(url);
}

void KMReaderWin::slotShowReader(KMime::Content *msgPart, bool html, const QString &encoding)
{
    const MessageViewer::Viewer::DisplayFormatMessage format = html ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
    KMReaderMainWin *win = new KMReaderMainWin(msgPart, format, encoding);
    win->show();
}

void KMReaderWin::slotShowMessage(KMime::Message::Ptr message, const QString &encoding)
{
    KMReaderMainWin *win = new KMReaderMainWin();
    win->showMessage(encoding, message);
    win->show();
}

void KMReaderWin::slotDeleteMessage(const Akonadi::Item &item)
{
    if (!item.isValid()) {
        return;
    }
    KMTrashMsgCommand *command = new KMTrashMsgCommand(item.parentCollection(), item, -1);
    command->start();
}

bool KMReaderWin::printSelectedText(bool preview)
{
    const QString str = mViewer->selectedText();
    if (str.isEmpty()) {
        return false;
    }
    ::MessageComposer::Composer *composer = new ::MessageComposer::Composer;
    composer->textPart()->setCleanPlainText(str);
    composer->textPart()->setWrappedPlainText(str);
    KMime::Message::Ptr messagePtr = message().payload<KMime::Message::Ptr>();
    composer->infoPart()->setFrom(messagePtr->from()->asUnicodeString());
    composer->infoPart()->setTo(QStringList() << messagePtr->to()->asUnicodeString());
    composer->infoPart()->setCc(QStringList() << messagePtr->cc()->asUnicodeString());
    composer->infoPart()->setSubject(messagePtr->subject()->asUnicodeString());
    composer->setProperty("preview", preview);
    connect(composer, &::MessageComposer::Composer::result, this, &KMReaderWin::slotPrintComposeResult);
    composer->start();
    return true;
}

void KMReaderWin::slotPrintComposeResult(KJob *job)
{
    const bool preview = job->property("preview").toBool();
    Q_ASSERT(dynamic_cast< ::MessageComposer::Composer * >(job));

    ::MessageComposer::Composer *composer = dynamic_cast< ::MessageComposer::Composer * >(job);
    if (composer->error() == ::MessageComposer::Composer::NoError) {

        Q_ASSERT(composer->resultMessages().size() == 1);
        Akonadi::Item printItem;
        printItem.setPayload<KMime::Message::Ptr>(composer->resultMessages().first());
        const bool useFixedFont = MessageViewer::GlobalSettings::self()->useFixedFont();
        const QString overrideEncoding = MessageCore::GlobalSettings::self()->overrideCharacterEncoding();

        KMPrintCommand *command = new KMPrintCommand(this, printItem, mViewer->headerStyle(), mViewer->headerStrategy()
                , mViewer->displayFormatMessageOverwrite(), mViewer->htmlLoadExternal() , useFixedFont, overrideEncoding);
        command->setPrintPreview(preview);
        command->start();
    } else {
        if (static_cast<KIO::Job *>(job)->ui()) {
            static_cast<KIO::Job *>(job)->ui()->showErrorMessage();
        } else {
            qWarning() << "Composer for printing failed:" << composer->errorString();
        }
    }

}

void KMReaderWin::clearContactItem()
{
    mSearchedContact = Akonadi::Item();
    mSearchedAddress = KContacts::Addressee();
    mLoadExternalReference->setChecked(false);
    mViewAsHtml->setChecked(false);
}

void KMReaderWin::setContactItem(const Akonadi::Item &contact, const KContacts::Addressee &address)
{
    mSearchedContact = contact;
    mSearchedAddress = address;
    updateHtmlActions();
}

void KMReaderWin::updateHtmlActions()
{
    if (!mSearchedContact.isValid()) {
        mLoadExternalReference->setChecked(false);
        mViewAsHtml->setChecked(false);
    } else {
        const QStringList customs = mSearchedAddress.customs();
        Q_FOREACH (const QString &custom, customs) {
            if (custom.contains(QLatin1String("MailPreferedFormatting"))) {
                const QString value = mSearchedAddress.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("MailPreferedFormatting"));
                mViewAsHtml->setChecked(value == QLatin1String("HTML"));
            } else if (custom.contains(QLatin1String("MailAllowToRemoteContent"))) {
                const QString value = mSearchedAddress.custom(QLatin1String("KADDRESSBOOK"), QLatin1String("MailAllowToRemoteContent"));
                mLoadExternalReference->setChecked((value == QLatin1String("TRUE")));
            }
        }
    }
}

void KMReaderWin::slotContactHtmlOptions()
{
    const KUrl url = urlClicked();
    if (url.isEmpty()) {
        return;
    }
    const QString emailString = KEmailAddress::decodeMailtoUrl(url).toLower();

    KPIM::AddEmailDiplayJob *job = new KPIM::AddEmailDiplayJob(emailString, mMainWindow, this);
    job->setRemoteContent(mLoadExternalReference->isChecked());
    job->setShowAsHTML(mViewAsHtml->isChecked());
    job->setContact(mSearchedContact);
    job->start();
}

void KMReaderWin::slotEditContact()
{
    if (mSearchedContact.isValid()) {
        QPointer<Akonadi::ContactEditorDialog> dlg =
            new Akonadi::ContactEditorDialog(Akonadi::ContactEditorDialog::EditMode, this);
        connect(dlg.data(), &Akonadi::ContactEditorDialog::contactStored, this, &KMReaderWin::contactStored);
        connect(dlg.data(), &Akonadi::ContactEditorDialog::error, this, &KMReaderWin::slotContactEditorError);
        dlg->setContact(mSearchedContact);
        dlg->exec();
        delete dlg;
    }
}

void KMReaderWin::slotContactEditorError(const QString &error)
{
    KMessageBox::error(this, i18n("Contact cannot be stored: %1", error), i18n("Failed to store contact"));
}

void KMReaderWin::contactStored(const Akonadi::Item &item)
{
    Q_UNUSED(item);
    KPIM::BroadcastStatus::instance()->setStatusMsg(i18n("Contact modified successfully"));
}

QAction *KMReaderWin::saveMessageDisplayFormatAction() const
{
    return mViewer->saveMessageDisplayFormatAction();
}

QAction *KMReaderWin::resetMessageDisplayFormatAction() const
{
    return mViewer->resetMessageDisplayFormatAction();
}

QAction *KMReaderWin::blockImage() const
{
    return mViewer->blockImage();
}

bool KMReaderWin::adblockEnabled() const
{
    return mViewer->adblockEnabled();
}

QAction *KMReaderWin::openBlockableItems() const
{
    return mViewer->openBlockableItems();
}


void KMReaderWin::slotShareImage()
{
    KMCommand *command = new KMShareImageCommand(imageUrlClicked(), this);
    command->start();
}

bool KMReaderWin::isAShortUrl(const QUrl &url) const
{
    return mViewer->isAShortUrl(url);
}

QAction *KMReaderWin::expandShortUrlAction() const
{
    return mViewer->expandShortUrlAction();
}

QAction *KMReaderWin::createTodoAction() const
{
    return mViewer->createTodoAction();
}

QAction *KMReaderWin::createEventAction() const
{
    return mViewer->createEventAction();
}

