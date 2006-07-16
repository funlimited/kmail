/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
//
// A toplevel KMainWindow derived class for displaying
// single messages or single message parts.
//
// Could be extended to include support for normal main window
// widgets like a toolbar.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <q3accel.h>
//Added by qt3to4:
#include <Q3PopupMenu>
#include <kactionmenu.h>
#include <kapplication.h>
#include <klocale.h>
#include <kstdaccel.h>
#include <kwin.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kstdaction.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include <kdebug.h>
#include "kmcommands.h"
#include "kmenubar.h"
#include "kmenu.h"
#include "kmreaderwin.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"

#include "kmreadermainwin.h"

KMReaderMainWin::KMReaderMainWin( bool htmlOverride, bool htmlLoadExtOverride,
                                  char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" ),
    mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  //mReaderWin->setShowCompleteMessage( true );
  mReaderWin->setAutoDelete( true );
  mReaderWin->setHtmlOverride( htmlOverride );
  mReaderWin->setHtmlLoadExtOverride( htmlLoadExtOverride );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
KMReaderMainWin::KMReaderMainWin( char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" ),
    mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  mReaderWin->setAutoDelete( true );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
KMReaderMainWin::KMReaderMainWin(KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QString & encoding, char *name )
  : KMail::SecondaryWindow( name ? name : "readerwindow#" ),
    mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this, actionCollection() );
  mReaderWin->setOverrideEncoding( encoding );
  mReaderWin->setMsgPart( aMsgPart, aHTML, aFileName, pname );
  initKMReaderMainWin();
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::initKMReaderMainWin() {
  setCentralWidget( mReaderWin );
  setupAccel();
  setupGUI( ToolBar | Keys | StatusBar | Create, "kmreadermainwin.rc" );
  applyMainWindowSettings( KMKernel::config(), "Separate Reader Window" );
  if( ! mReaderWin->message() ) { 
    menuBar()->hide(); 
    toolBar( "mainToolBar" )->hide(); 
  } 

  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );
}

//-----------------------------------------------------------------------------
KMReaderMainWin::~KMReaderMainWin()
{
  saveMainWindowSettings( KMKernel::config(), "Separate Reader Window" );
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::setUseFixedFont( bool useFixedFont )
{
  mReaderWin->setUseFixedFont( useFixedFont );
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::showMsg( const QString & encoding, KMMessage *msg )
{
  mReaderWin->setOverrideEncoding( encoding );
  mReaderWin->setMsg( msg, true );
  setCaption( msg->subject() );
  mMsg = msg;
  menuBar()->show();
  toolBar( "mainToolBar" )->show();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotPrintMsg()
{
  KMCommand *command = new KMPrintCommand( this, mReaderWin->message(),
      mReaderWin->htmlOverride(), mReaderWin->htmlLoadExtOverride(),
      mReaderWin->isFixedFont(), mReaderWin->overrideEncoding() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyToMsg()
{
  KMCommand *command = new KMReplyToCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyAuthorToMsg()
{
  KMCommand *command = new KMReplyAuthorCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyAllToMsg()
{
  KMCommand *command = new KMReplyToAllCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotReplyListToMsg()
{
  KMCommand *command = new KMReplyListCommand( this, mReaderWin->message(),
      mReaderWin->copyText() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotForwardMsg()
{
   KMCommand *command = 0;
   if ( mReaderWin->message() && mReaderWin->message()->parent() ) {
    command = new KMForwardCommand( this, mReaderWin->message(),
        mReaderWin->message()->parent()->identity() );
   } else {
    command = new KMForwardCommand( this, mReaderWin->message() );
   }
   command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotForwardAttachedMsg()
{
   KMCommand *command = 0;
   if ( mReaderWin->message() && mReaderWin->message()->parent() ) {
     command = new KMForwardAttachedCommand( this, mReaderWin->message(),
        mReaderWin->message()->parent()->identity() );
   } else {
     command = new KMForwardAttachedCommand( this, mReaderWin->message() );
   }
   command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotRedirectMsg()
{
  KMCommand *command = new KMRedirectCommand( this, mReaderWin->message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotShowMsgSrc()
{
  KMMessage *msg = mReaderWin->message();
  if ( !msg )
    return;
  KMCommand *command = new KMShowMsgSrcCommand( this, msg,
                                                mReaderWin->isFixedFont() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderMainWin::slotConfigChanged()
{
  //readConfig();
}

void KMReaderMainWin::setupAccel()
{
  if ( kmkernel->xmlGuiInstance() )
    setInstance( kmkernel->xmlGuiInstance() );

  //----- File Menu
  //mOpenAction = KStdAction::open( this, SLOT( slotOpenMsg() ),
  //                                actionCollection() );

  //mSaveAsAction = new KAction( i18n("Save &As..."), "filesave",
  //                             KStdAccel::shortcut( KStdAccel::Save ),
  //                             this, SLOT( slotSaveMsg() ),
  //                             actionCollection(), "file_save_as" );

  mPrintAction = KStdAction::print( this, SLOT( slotPrintMsg() ),
                                    actionCollection() );

  KAction *closeAction = KStdAction::close( this, SLOT( close() ), actionCollection() );
  KShortcut closeShortcut = closeAction->shortcut();
  closeShortcut.append( QKeySequence(Qt::Key_Escape));
  closeAction->setShortcut(closeShortcut);

  //----- View Menu
  mViewSourceAction = new KAction( i18n("&View Source"), actionCollection(), "view_source" );
  connect(mViewSourceAction, SIGNAL(triggered(bool) ), SLOT(slotShowMsgSrc()));
  mViewSourceAction->setShortcut(Qt::Key_V);


  mForwardActionMenu = new KActionMenu( KIcon("mail_forward"),
                                        i18nc("Message->","&Forward"),
					actionCollection(),
					"message_forward" );
  connect( mForwardActionMenu, SIGNAL( activated() ), this,
           SLOT( slotForwardMsg() ) );

  mForwardAction = new KAction(KIcon("mail_forward"),  i18n("&Inline..."), actionCollection(), "message_forward_inline" );
  connect(mForwardAction, SIGNAL(triggered(bool) ), SLOT(slotForwardMsg()));
  mForwardAction->setShortcut(Qt::SHIFT+Qt::Key_F);
  mForwardActionMenu->insert( mForwardAction );

  mForwardAttachedAction = new KAction( KIcon("mail_forward"), i18nc("Message->Forward->","As &Attachment..."),
				       actionCollection(), "message_forward_as_attachment" );
  mForwardAttachedAction->setShortcut(Qt::Key_F);
  connect(mForwardAttachedAction, SIGNAL(triggered(bool) ), SLOT(slotForwardAttachedMsg()));
  mForwardActionMenu->insert( mForwardAttachedAction );

  mRedirectAction = new KAction( i18nc("Message->Forward->","&Redirect..."),
				 Qt::Key_E, this, SLOT(slotRedirectMsg()), actionCollection(), "message_forward_redirect" );
  mForwardActionMenu->insert( mRedirectAction );

  mReplyActionMenu = new KActionMenu( KIcon("mail_reply"), i18nc("Message->","&Reply"),
                                      actionCollection(),
                                      "message_reply_menu" );
  connect( mReplyActionMenu, SIGNAL(activated()), this,
	   SLOT(slotReplyToMsg()) );

  mReplyAction = new KAction(KIcon("mail_reply"),  i18n("&Reply..."), actionCollection(), "reply" );
  connect(mReplyAction, SIGNAL(triggered(bool)), SLOT(slotReplyToMsg()));
  mReplyAction->setShortcut(Qt::Key_R);
  mReplyActionMenu->insert( mReplyAction );

  mReplyAuthorAction = new KAction(KIcon("mail_reply"),  i18n("Reply to A&uthor..."), actionCollection(), "reply_author" );
  connect(mReplyAuthorAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAuthorToMsg()));
  mReplyAuthorAction->setShortcut(Qt::SHIFT+Qt::Key_A);
  mReplyActionMenu->insert( mReplyAuthorAction );

  mReplyAllAction = new KAction(KIcon("mail_replyall"),  i18n("Reply to &All..."), actionCollection(), "reply_all" );
  connect(mReplyAllAction, SIGNAL(triggered(bool) ), SLOT(slotReplyAllToMsg()));
  mReplyAllAction->setShortcut(Qt::Key_A);
  mReplyActionMenu->insert( mReplyAllAction );

  mReplyListAction = new KAction(KIcon("mail_replylist"),  i18n("Reply to Mailing-&List..."), actionCollection(), "reply_list" );
  connect(mReplyListAction, SIGNAL(triggered(bool) ), SLOT(slotReplyListToMsg()));
  mReplyListAction->setShortcut(Qt::Key_L);
  mReplyActionMenu->insert( mReplyListAction );



  Q3Accel *accel = new Q3Accel(mReaderWin, "showMsg()");
  accel->connectItem(accel->insertItem(Qt::Key_Up),
                     mReaderWin, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Qt::Key_Down),
                     mReaderWin, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Qt::Key_PageUp),
                     mReaderWin, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Qt::Key_PageDown),
                     mReaderWin, SLOT(slotScrollNext()));
  accel->connectItem(accel->insertItem(KStdAccel::shortcut(KStdAccel::Copy)),
                     mReaderWin, SLOT(slotCopySelectedText()));
  connect( mReaderWin, SIGNAL(popupMenu(KMMessage&,const KUrl&,const QPoint&)),
	  this, SLOT(slotMsgPopup(KMMessage&,const KUrl&,const QPoint&)));
  connect(mReaderWin, SIGNAL(urlClicked(const KUrl&,int)),
	  mReaderWin, SLOT(slotUrlClicked()));

}


void KMReaderMainWin::slotMsgPopup(KMMessage &aMsg, const KUrl &aUrl, const QPoint& aPoint)
{
  KMenu * menu = new KMenu;
  mUrl = aUrl;
  mMsg = &aMsg;
  bool urlMenuAdded=false;

  if (!aUrl.isEmpty())
  {
    if (aUrl.protocol() == "mailto") {
      // popup on a mailto URL
      menu->addAction( mReaderWin->mailToComposeAction() );
      if ( mMsg ) {
        menu->addAction( mReaderWin->mailToReplyAction() );
        menu->addAction( mReaderWin->mailToForwardAction() );
        menu->addSeparator();
      }
      menu->addAction( mReaderWin->addAddrBookAction() );
      menu->addAction( mReaderWin->openAddrBookAction() );
      menu->addAction( mReaderWin->copyAction() );
    } else {
      // popup on a not-mailto URL
      menu->addAction( mReaderWin->urlOpenAction() );
      menu->addAction( mReaderWin->addBookmarksAction() );
      menu->addAction( mReaderWin->urlSaveAsAction() );
      menu->addAction( mReaderWin->copyURLAction() );
    }
    urlMenuAdded=true;
  }
  if(!mReaderWin->copyText().isEmpty()) {
    if ( urlMenuAdded )
      menu->addSeparator();
    menu->addAction( mReaderWin->copyAction() );
    menu->addAction( mReaderWin->selectAllAction() );
  } else if ( !urlMenuAdded )
  {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mMsg) // no message
    {
      delete menu;
      return;
    }

    if( !aMsg.parent()->isSent() && !aMsg.parent()->isDrafts() ) {
      menu->addAction( mReplyActionMenu );
      menu->addAction( mForwardActionMenu );
      menu->addSeparator();
    }

    Q3PopupMenu* copyMenu = new Q3PopupMenu(menu);
    KMMainWidget* mainwin = kmkernel->getKMMainWidget();
    if ( mainwin )
      mainwin->folderTree()->folderToPopupMenu( KMFolderTree::CopyMessage, this,
          &mMenuToFolder, copyMenu );
    menu->insertItem( i18n("&Copy To" ), copyMenu );
    menu->addSeparator();
    menu->addAction( mViewSourceAction );
    menu->addAction( mReaderWin->toggleFixFontAction() );
    menu->addSeparator();
    menu->addAction( mPrintAction );
    menu->insertItem(  SmallIcon("filesaveas"), i18n( "Save &As..." ), mReaderWin, SLOT( slotSaveMsg() ) );
    menu->insertItem( i18n("Save Attachments..."), mReaderWin, SLOT(slotSaveAttachments()) );
  }
  menu->exec(aPoint, 0);
  delete menu;
}

void KMReaderMainWin::copySelectedToFolder( int menuId )
{
  if (!mMenuToFolder[menuId])
    return;

  KMCommand *command = new KMCopyCommand( mMenuToFolder[menuId], mMsg );
  command->start();
}

#include "kmreadermainwin.moc"
