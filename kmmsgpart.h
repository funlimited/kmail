/* part of a mime multi-part message
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmmsgpart_h
#define kmmsgpart_h

#include <qstring.h>
#include <qcstring.h>

template <typename T>
class QValueList;

class KMMessagePart
{
public:
  static QString encodeBase64(const QString& aStr);

  KMMessagePart();
  virtual ~KMMessagePart();

  /** Get or set the message body */
  QCString body(void) const;
  void setBody(const QCString &aStr);

  /** Returns body as decoded string. Assumes that content-transfer-encoding
    contains the correct encoding. This routine is meant for binary data.
    No trailing 0 is appended. */
  virtual QByteArray bodyDecodedBinary(void) const;

  /** Returns body as decoded string. Assumes that content-transfer-encoding
      contains the correct encoding. This routine is meant for text strings! */
  virtual QCString bodyDecoded(void) const;

  /** Sets body, encoded in the best fitting
      content-transfer-encoding, which is determined by character
      frequency count.
      
      @param aBuf       input buffer
      @param allowedCte return: list of allowed cte's
      @param allow8Bit  whether "8bit" is allowed as cte.
  */
  virtual void setBodyAndGuessCte(const QByteArray& aBuf,
				  QValueList<int>& allowedCte,
				  bool allow8Bit=false);
  /* Same for text */
  virtual void setBodyAndGuessCte(const QCString& aBuf,
				  QValueList<int>& allowedCte,
				  bool allow8Bit=false);

  /** Sets body, encoded according to the content-transfer-encoding.
      BEWARE: The entire aStr is used including trailing 0 of text strings! */
  virtual void setBodyEncodedBinary(const QByteArray& aStr);

  /** Sets body, encoded according to the content-transfer-encoding.
      This one is for text strings, the trailing 0 is not used. */
  virtual void setBodyEncoded(const QCString& aStr);

  /** Returns decoded length of body. */
  virtual int decodedSize(void) const;

  /** Get or set name parameter */
  QString name(void) const;
  void setName(const QString& aStr);

  /** Get or set the 'Content-Type' header field
   The member functions that involve enumerated types (ints)
   will work only for well-known types or subtypes. */
  QCString originalContentTypeStr(void) const { return mOriginalContentTypeStr; }
  void setOriginalContentTypeStr( const QCString& txt )
  {
    mOriginalContentTypeStr = txt;
  }
  QCString typeStr(void) const;
  int type(void) const;
  void setTypeStr(const QCString &aStr);
  void setType(int aType);
  /** Subtype */
  QCString subtypeStr(void) const;
  int subtype(void) const;
  void setSubtypeStr(const QCString &aStr);
  void setSubtype(int aSubtype);

  /** Set the 'Content-Type' by mime-magic from the contents of the body.
    If autoDecode is TRUE the decoded body will be used for mime type
    determination (this does not change the body itself). */
  void magicSetType(bool autoDecode=TRUE);

  /** Get or set a custom content type parameter, consisting of an attribute
    name and a corresponding value. */
  QCString parameterAttribute(void) const;
  QString parameterValue(void) const;
  void setParameter(const QCString &attribute, const QString &value);

  QCString additionalCTypeParamStr(void) const
  {
    return mAdditionalCTypeParamStr;
  }
  void setAdditionalCTypeParamStr( const QCString &param )
  {
    mAdditionalCTypeParamStr = param;
  }

  /** Tries to find a good icon for the 'Content-Type' by scanning
    the installed mimelnk files. Returns the found icon. If no matching
    icon is found, the one for application/octet-stream is returned. */
  QString iconName(const QString &mimeType = QString::null) const;

  /** Get or set the 'Content-Transfer-Encoding' header field
    The member functions that involve enumerated types (ints)
    will work only for well-known encodings. */
  QCString contentTransferEncodingStr(void) const;
  int  contentTransferEncoding(void) const;
  void setContentTransferEncodingStr(const QCString &aStr);
  void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names. */
  QCString cteStr(void) const { return contentTransferEncodingStr(); }
  int cte(void) const { return contentTransferEncoding(); }
  void setCteStr(const QCString& aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }


  /** Get or set the 'Content-Description' header field */
  QString contentDescription() const;
  QCString contentDescriptionEncoded() const { return mContentDescription; }
  void setContentDescription(const QString &aStr);

  /** Get or set the 'Content-Disposition' header field */
  QCString contentDisposition() const;
  void setContentDisposition(const QCString &aStr);

  /** Get the message part charset.*/
  virtual QCString charset(void) const;

  /** Set the message part charset. */
  virtual void setCharset(const QCString &aStr);

  /** Returns name of filename part of 'Content-Disposition' header field,
      if present. */
  QString fileName(void) const;

protected:
  QCString mOriginalContentTypeStr;
  QCString mType;
  QCString mSubtype;
  QCString mCte;
  QCString mContentDescription;
  QCString mContentDisposition;
  QByteArray mBody;
  QCString mAdditionalCTypeParamStr;
  QString mName;
  QCString mParameterAttribute;
  QString mParameterValue;
  QCString mCharset;
  mutable int mBodyDecodedSize;
};


#endif /*kmmsgpart_h*/
