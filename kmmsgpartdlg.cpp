// kmmsgpartdlg.cpp


// my includes:
#include "kmmsgpartdlg.h"

// other KMail includes:
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmglobal.h"
#include "kbusyptr.h"

// other kdenetwork includes: (none)

// other KDE includes:
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kaboutdata.h>
#include <kstringvalidator.h>
#include <kcombobox.h>
#include <kdebug.h>

// other Qt includes:
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcombobox.h>
#include <qwhatsthis.h>
#include <qlineedit.h>
#include <qcheckbox.h>

// other includes:
#include <assert.h>

static const struct {
  KMMsgPartDialog::Encoding encoding;
  const char * displayName;
} encodingTypes[] = {
  { KMMsgPartDialog::SevenBit, I18N_NOOP("none (7bit text)") },
  { KMMsgPartDialog::EightBit, I18N_NOOP("none (8bit text)") },
  { KMMsgPartDialog::QuotedPrintable, I18N_NOOP("quoted printable") },
  { KMMsgPartDialog::Base64, I18N_NOOP("base 64") },
};
static const int numEncodingTypes =
  sizeof encodingTypes / sizeof *encodingTypes;

KMMsgPartDialog::KMMsgPartDialog( const QString & caption,
				  QWidget * parent, const char * name )
  : KDialogBase( Plain,
		 caption.isEmpty() ? i18n("Message Part Properties") : caption,
		 Ok|Cancel|Help, Ok, parent, name )
{
  // tmp vars:
  QGridLayout * glay;
  QLabel      * label;
  QString       msg;

  setHelp( QString::fromLatin1("attachments") );

  mUnknownPixmap = kapp->iconLoader()->loadIcon( "unknown", KIcon::Desktop );

  for ( int i = 0 ; i < numEncodingTypes ; ++i )
    mI18nizedEncodings << i18n( encodingTypes[i].displayName );

  glay = new QGridLayout( plainPage(), 9 /*rows*/, 2 /*cols*/, spacingHint() );
  glay->setColStretch( 1, 1 );
  glay->setRowStretch( 8, 1 );

  // mimetype icon:
  mIcon = new QLabel( plainPage() );
  mIcon->setPixmap( mUnknownPixmap );
  glay->addMultiCellWidget( mIcon, 0, 1, 0, 0 );

  // row 0: Type combobox:
  mMimeType = new KComboBox( true, plainPage() );
  mMimeType->setInsertionPolicy( QComboBox::NoInsertion );
  mMimeType->setValidator( new KMimeTypeValidator( mMimeType ) );
  mMimeType->insertStringList( QStringList()
			       << QString::fromLatin1("text/html")
			       << QString::fromLatin1("text/plain")
			       << QString::fromLatin1("image/gif")
			       << QString::fromLatin1("image/jpeg")
			       << QString::fromLatin1("image/png")
			       << QString::fromLatin1("application/octet-stream")
			       << QString::fromLatin1("application/x-gunzip")
			       << QString::fromLatin1("application/zip") );
  connect( mMimeType, SIGNAL(textChanged(const QString&)),
	   this, SLOT(slotMimeTypeChanged(const QString&)) );
  glay->addWidget( mMimeType, 0, 1 );

  msg = i18n("<qt><p>The <em>mime type</em> of the file.</p>"
	     "<p>Normally, you don't need to touch this setting, since the "
	     "type of the file is automatically checked. But sometimes, %1 "
	     "may not detect the type correctly. Here's where you can fix "
	     "that.</p></qt>").arg( kapp->aboutData()->programName() );
  QWhatsThis::add( mMimeType, msg );

  // row 1: Size label:
  mSize = new QLabel( plainPage() );
  setSize( KIO::filesize_t(0) );
  glay->addWidget( mSize, 1, 1 );

  msg = i18n("<qt><p>The size of the part.</p>"
	     "<p>Sometimes, %1 will only give an estimated size here, "
	     "because calculating the exact size would take too much time. "
	     "When this is the case, it will be made visible by adding "
	     "\"(est.)\" to the size displayed.</p></qt>")
    .arg( kapp->aboutData()->programName() );
  QWhatsThis::add( mSize, msg );

  // row 2: "Name" lineedit and label:
  mFileName = new QLineEdit( plainPage() );
  label = new QLabel( mFileName, i18n("&Name:"), plainPage() );
  glay->addWidget( label, 2, 0 );
  glay->addWidget( mFileName, 2, 1 );

  msg = i18n("<qt><p>The file name of the part.</p>"
	     "<p>Although this defaults to the name of the attached file, "
	     "it doesn't specify the file to be attached. Rather, it "
	     "suggests a file name to be used by the recipient's mail agent "
	     "when saving the part to disk.</p></qt>");
  QWhatsThis::add( label, msg );
  QWhatsThis::add( mFileName, msg );

  // row 3: "Description" lineedit and label:
  mDescription = new QLineEdit( plainPage() );
  label = new QLabel( mDescription, i18n("&Description:"), plainPage() );
  glay->addWidget( label, 3, 0 );
  glay->addWidget( mDescription, 3, 1 );

  msg = i18n("<qt><p>A description of the part.</p>"
	     "<p>This is just an informational description of the part, "
	     "much like the Subject is for the whole message. Most "
	     "mail agents will show this information in their message "
	     "previews, alongside the attachment's icon.</p></qt>");
  QWhatsThis::add( label, msg );
  QWhatsThis::add( mDescription, msg );

  // row 4: "Encoding" combobox and label:
  mEncoding = new QComboBox( false, plainPage() );
  mEncoding->insertStringList( mI18nizedEncodings );
  label = new QLabel( mEncoding, i18n("&Encoding"), plainPage() );
  glay->addWidget( label, 4, 0 );
  glay->addWidget( mEncoding, 4, 1 );

  msg = i18n("<qt><p>The transport encoding of this part.</p>"
	     "<p>Normally, you don't need to change this, since %1 will use "
	     "a decent default encoding, depending on the mime type. Yet "
	     "sometimes, you can significantly reduce the size of the "
	     "resulting message, e.g. if a PostScript file doesn't contain "
	     "binary data, but consists of pure text. In this case, choosing "
	     "\"quoted-printable\" over the default \"base64\" will save up "
	     "to 25% in resulting message size.</p></qt>");
  QWhatsThis::add( label, msg );
  QWhatsThis::add( mEncoding, msg );

  // row 5: "Suggest automatic display..." checkbox:
  mInline = new QCheckBox( i18n("Suggest &automatic display"), plainPage() );
  glay->addMultiCellWidget( mInline, 5, 5, 0, 1 );

  msg = i18n("<qt><p>Check this option if you want to suggest to the "
	     "recipient the automatic (inline) display of this part in the "
	     "message preview instead of the default icon view.</p>"
	     "<p>Technically, this is carried out by setting this part's "
	     "<em>Content-Disposition</em> header field to \"inline\" "
	     "instead of the default \"attachment\".</p></qt>");
  QWhatsThis::add( mInline, msg );

#ifdef AEGYPTEN
  // row 6: "Sign" checkbox:
  mSigned = new QCheckBox( i18n("&Sign this part"), plainPage() );
  glay->addMultiCellWidget( mSigned, 6, 6, 0, 1 );

  msg = i18n("<qt><p>Check this option if you want this message part to be "
	     "signed.</p>"
	     "<p>The signature will be made with the key that you associated "
	     "with the currently selected identity.</p></qt>");
  QWhatsThis::add( mSigned, msg );

  // row 7: "Encrypt" checkbox:
  mEncrypted = new QCheckBox( i18n("Encr&ypt this part"), plainPage() );
  glay->addMultiCellWidget( mEncrypted, 7, 7, 0, 1 );

  msg = i18n("<qt><p>Check this option if you want this message part to be "
	     "encrypted.</p>"
	     "<p>The part will be encrypted for the recipients of this "
	     "message</p></qt>");
  QWhatsThis::add( mEncrypted, msg );
  // (row 8: spacer)
#else
  mSigned = mEncrypted = 0;
#endif
}


KMMsgPartDialog::~KMMsgPartDialog() {}


QString KMMsgPartDialog::mimeType() const {
  return mMimeType->currentText();
}

void KMMsgPartDialog::setMimeType( const QString & mimeType ) {
  int dummy = 0;
  QString tmp = mimeType; // get rid of const'ness
  if ( mMimeType->validator()->validate( tmp, dummy ) )
    for ( int i = 0 ; i < mMimeType->count() ; ++i )
      if ( mMimeType->text( i ) == mimeType ) {
	mMimeType->setCurrentItem( i );
	return;
      }
  mMimeType->insertItem( mimeType, 0 );
  mMimeType->setCurrentItem( 0 );
  slotMimeTypeChanged( mimeType );
}

void KMMsgPartDialog::setMimeType( const QString & type,
				   const QString & subtype ) {
  setMimeType( QString::fromLatin1("%1/%2").arg(type).arg(subtype) );
}

void KMMsgPartDialog::setMimeTypeList( const QStringList & mimeTypes ) {
  mMimeType->insertStringList( mimeTypes );
}

void KMMsgPartDialog::setSize( KIO::filesize_t size, bool estimated ) {
  QString text = KIO::convertSize( size );
  if ( estimated )
    text = i18n("%1: a filesize incl. unit (e.g. \"1.3 KB\")",
		"%1 (est.)").arg( text );
  mSize->setText( text );
}

QString KMMsgPartDialog::fileName() const {
  return mFileName->text();
}

void KMMsgPartDialog::setFileName( const QString & fileName ) {
  mFileName->setText( fileName );
}

QString KMMsgPartDialog::description() const {
  return mDescription->text();
}

void KMMsgPartDialog::setDescription( const QString & description ) {
  mDescription->setText( description );
}

KMMsgPartDialog::Encoding KMMsgPartDialog::encoding() const {
  QString s = mEncoding->currentText();
  for ( unsigned int i = 0 ; i < mI18nizedEncodings.count() ; ++i )
    if ( s == *mI18nizedEncodings.at(i) )
      return encodingTypes[i].encoding;
  kdFatal(5006) << "KMMsgPartDialog::encoding(): Unknown encoding encountered!"
		<< endl;
  return None; // keep compiler happy
}

void KMMsgPartDialog::setEncoding( Encoding encoding ) {
  for ( int i = 0 ; i < numEncodingTypes ; ++i )
    if ( encodingTypes[i].encoding == encoding ) {
      QString text = *mI18nizedEncodings.at(i);
      for ( int j = 0 ; j < mEncoding->count() ; ++j )
	if ( mEncoding->text(j) == text ) {
	  mEncoding->setCurrentItem( j );
	  return;
	}
      mEncoding->insertItem( text, 0 );
      mEncoding->setCurrentItem( 0 );
    }
  kdFatal(5006) << "KMMsgPartDialog::setEncoding(): "
    "Unknown encoding encountered!" << endl;
}

void KMMsgPartDialog::setShownEncodings( int encodings ) {
  mEncoding->clear();
  for ( int i = 0 ; i < numEncodingTypes ; ++i )
    if ( encodingTypes[i].encoding & encodings )
      mEncoding->insertItem( *mI18nizedEncodings.at(i) );
}

bool KMMsgPartDialog::isInline() const {
  return mInline->isChecked();
}

void KMMsgPartDialog::setInline( bool inlined ) {
  mInline->setChecked( inlined );
}

bool KMMsgPartDialog::isEncrypted() const {
#ifdef AEGYPTEN
  return mEncrypted->isChecked();
#else
  return false;
#endif
}

void KMMsgPartDialog::setEncrypted( bool encrypted ) {
#ifdef AEGYPTEN
  mEncrypted->setChecked( encrypted );
#else
  (void)encrypted;
#endif
}

void KMMsgPartDialog::setCanEncrypt( bool enable ) {
#ifdef AEGYPTEN
  mEncrypted->setEnabled( enable );
#else
  (void)enable;
#endif
}

bool KMMsgPartDialog::isSigned() const {
#ifdef AEGYPTEN
  return mSigned->isChecked();
#else
  return false;
#endif
}

void KMMsgPartDialog::setSigned( bool sign ) {
#ifdef AEGYPTEN
  mSigned->setChecked( sign );
#else
  (void)sign;
#endif
}

void KMMsgPartDialog::setCanSign( bool enable ) {
#ifdef AEGYPTEN
  mSigned->setEnabled( enable );
#else
  (void)enable;
#endif
}

void KMMsgPartDialog::slotMimeTypeChanged( const QString & mimeType ) {
  // message subparts MUST have 7bit encoding...
  if ( mimeType.startsWith("message/") ) {
    setEncoding( SevenBit );
    mEncoding->setEnabled( false );
  } else {
    mEncoding->setEnabled( true );
  }
  // find a mimetype icon:
  int dummy = 0;
  QString tmp = mimeType; // get rid of const'ness
  if ( mMimeType->validator()->validate( tmp, dummy )
       == QValidator::Acceptable )
    mIcon->setPixmap( KMimeType::mimeType( mimeType )->pixmap( KIcon::Desktop ) );
  else
    mIcon->setPixmap( mUnknownPixmap );
}




KMMsgPartDialogCompat::KMMsgPartDialogCompat( const char *, bool )
  : KMMsgPartDialog(), mMsgPart( 0 )
{
  setShownEncodings( SevenBit|EightBit|QuotedPrintable|Base64 );
}

KMMsgPartDialogCompat::~KMMsgPartDialogCompat() {}

void KMMsgPartDialogCompat::setMsgPart( KMMessagePart * aMsgPart )
{
  mMsgPart = aMsgPart;
  assert( mMsgPart );

  QCString enc = mMsgPart->cteStr();
  if ( enc == "7bit" )
    setEncoding( SevenBit );
  else if ( enc == "8bit" )
    setEncoding( EightBit );
  else if ( enc == "quoted-printable" )
    setEncoding( QuotedPrintable );
  else
    setEncoding( Base64 );

  setDescription( mMsgPart->contentDescription() );
  setFileName( mMsgPart->fileName() );
  setMimeType( mMsgPart->typeStr(), mMsgPart->subtypeStr() );
  setSize( mMsgPart->decodedSize() );
  setInline( mMsgPart->contentDisposition()
	     .find( QRegExp("^\\s*inline", false) ) >= 0 );
}


void KMMsgPartDialogCompat::applyChanges()
{
  if (!mMsgPart) return;

  kernel->kbp()->busy();

  // apply Content-Disposition:
  QCString cDisp;
  if ( isInline() )
    cDisp = "inline;";
  else
    cDisp = "attachment;";

  QString name = fileName();
  if ( !name.isEmpty() || !mMsgPart->name().isEmpty()) {
    mMsgPart->setName( name );
    QCString encoding = KMMessage::autoDetectCharset( mMsgPart->charset(),
      KMMessage::preferredCharsets(), name );
    if ( encoding.isEmpty() ) encoding = "utf-8";
    QCString encName = KMMsgBase::encodeRFC2231String( name, encoding );

    cDisp += " filename";
    if ( name != QString( encName ) )
      cDisp += '*';
    cDisp += "=\"" + encName + '"';
    mMsgPart->setContentDisposition( cDisp );
  }

  // apply Content-Description"
  QString desc = description();
  if ( !desc.isEmpty() || !mMsgPart->contentDescription().isEmpty() )
    mMsgPart->setContentDescription( desc );

  // apply Content-Type:
  QCString type = mimeType().latin1();
  QCString subtype;
  int idx = type.find('/');
  if ( idx < 0 )
    subtype = "";
  else {
    subtype = type.mid( idx+1 );
    type = type.left( idx );
  }
  mMsgPart->setTypeStr(type);
  mMsgPart->setSubtypeStr(subtype);

  // apply Content-Transfer-Encoding:
  QCString cte;
  if (subtype == "rfc822" && type == "message")
    kdWarning( encoding() != SevenBit && encoding() != EightBit, 5006 ) 
      << "encoding on rfc822/message must be \"7bit\" or \"8bit\"" << endl;
  switch ( encoding() ) {
  case SevenBit:        cte = "7bit";             break;
  case EightBit:        cte = "8bit";             break;
  case QuotedPrintable: cte = "quoted-printable"; break;
  case Base64: default: cte = "base64";           break;
  }
  if ( cte != mMsgPart->cteStr().lower() ) {
    QByteArray body = mMsgPart->bodyDecodedBinary();
    mMsgPart->setCteStr( cte );
    mMsgPart->setBodyEncodedBinary( body );
  }


  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMMsgPartDialogCompat::slotOk()
{
  applyChanges();
  KMMsgPartDialog::slotOk();
}


//-----------------------------------------------------------------------------
#include "kmmsgpartdlg.moc"
