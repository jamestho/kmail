// -*- mode: C++; c-file-style: "gnu" -*-
// Header for kmreaderwin the kmail reader
// written by Markus Wuebben <markus.wuebben@kde.org>

#ifndef KMREADERWIN_H
#define KMREADERWIN_H

#include <qwidget.h>
#include <qtimer.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QEvent>
#include <QList>
#include <QResizeEvent>
#include <kurl.h>
#include <kservice.h>
#include <messagestatus.h>
#include <kvbox.h>
using KPIM::MessageStatus;
#include "kmmimeparttree.h" // Needed for friend declaration.
#include "interfaces/observer.h"

class QSplitter;
class KHBox;
class Q3ListViewItem;
class QScrollBar;
class QString;
class Q3TabDialog;
class QTextCodec;

class DwHeaders;
class DwMediaType;

class KActionCollection;
class KAction;
class KActionMenu;
class KSelectAction;
class KToggleAction;
class KToggleAction;
class KConfigBase;
class KHTMLPart;
class KUrl;

class KMFolder;
class KMMessage;
class KMMessagePart;
namespace KMail {
  namespace Interface {
    class Observable;
  }
  class PartMetaData;
  class ObjectTreeParser;
  class AttachmentStrategy;
  class HeaderStrategy;
  class HeaderStyle;
  class HtmlWriter;
  class KHtmlPartHtmlWriter;
  class ISubject;
  class HtmlStatusBar;
  class FolderJob;
  class CSSHelper;
}

class partNode; // might be removed when KMime is used instead of mimelib
                //                                      (khz, 29.11.2001)

namespace KParts {
  struct URLArgs;
}

/**
   This class implements a "reader window", that is a window
   used for reading or viewing messages.
*/

class KMReaderWin: public QWidget, public KMail::Interface::Observer {
  Q_OBJECT

  friend void KMMimePartTree::itemClicked( Q3ListViewItem* item );
  friend void KMMimePartTree::itemRightClicked( Q3ListViewItem* item, const QPoint & );
  friend void KMMimePartTree::slotSaveAs();

  friend class KMail::ObjectTreeParser;
  friend class KMail::KHtmlPartHtmlWriter;

public:
  KMReaderWin( QWidget *parent,
	       QWidget *mainWindow,
	       KActionCollection *actionCollection,
               const char *name=0,
	       Qt::WindowFlags f=0 );
  virtual ~KMReaderWin();

  /**
     \reimp from Interface::Observer
     Updates the current message
   */
  void update( KMail::Interface::Observable * );

  /** Read settings from app's config file. */
  void readConfig();

  /** Write settings to app's config file. Calls sync() if withSync is TRUE. */
  void writeConfig( bool withSync=true ) const;

  const KMail::HeaderStyle * headerStyle() const {
    return mHeaderStyle;
  }
  /** Set the header style and strategy. We only want them to be set
      together. */
  void setHeaderStyleAndStrategy( const KMail::HeaderStyle * style,
				  const KMail::HeaderStrategy * strategy );

  /** Getthe message header strategy. */
  const KMail::HeaderStrategy * headerStrategy() const {
    return mHeaderStrategy;
  }

  /** Get/set the message attachment strategy. */
  const KMail::AttachmentStrategy * attachmentStrategy() const {
    return mAttachmentStrategy;
  }
  void setAttachmentStrategy( const KMail::AttachmentStrategy * strategy );

  /** Get selected override character encoding.
      @return The encoding selected by the user or an empty string if auto-detection
      is selected. */
  QString overrideEncoding() const { return mOverrideEncoding; }

  /** Set the override character encoding. */
  void setOverrideEncoding( const QString & encoding );

  /** Get codec corresponding to the currently selected override character encoding.
      @return The override codec or 0 if auto-detection is selected. */
  const QTextCodec * overrideCodec() const;

  /** Set printing mode */
  virtual void setPrinting(bool enable) { mPrinting = enable; }

  /** Set the message that shall be shown. If msg is 0, an empty page is
      displayed. */
  virtual void setMsg(KMMessage* msg, bool force = false);

  /** Instead of settings a message to be shown sets a message part
      to be shown */
  void setMsgPart( KMMessagePart* aMsgPart, bool aHTML,
		   const QString& aFileName, const QString& pname );

  void setMsgPart( partNode * node );

  /** Show or hide the Mime Tree Viewer if configuration
      is set to smart mode.  */
  void showHideMimeTree( bool isPlainTextTopLevel );

  /** Store message id of last viewed message,
      normally no need to call this function directly,
      since correct value is set automatically in
      parseMsg(KMMessage* aMsg, bool onlyProcessHeaders). */
  void setIdOfLastViewedMessage( const QString & msgId )
    { mIdOfLastViewedMessage = msgId; }

  /** Clear the reader and discard the current message. */
  void clear(bool force = false) { setMsg(0, force); }

  /** Re-parse the current message. */
  void update(bool force = false);

  /** Print current message. */
  virtual void printMsg(void);

  /** Return selected text */
  QString copyText();

  /** Get/set auto-delete msg flag. */
  bool autoDelete(void) const { return mAutoDelete; }
  void setAutoDelete(bool f) { mAutoDelete=f; }

  /** Override default html mail setting */
  bool htmlOverride() const { return mHtmlOverride; }
  void setHtmlOverride( bool override );

  /** Override default load external references setting */
  bool htmlLoadExtOverride() const { return mHtmlLoadExtOverride; }
  void setHtmlLoadExtOverride( bool override );

  /** Is html mail to be supported? Takes into account override */
  bool htmlMail();

  /** Is loading ext. references to be supported? Takes into account override */
  bool htmlLoadExternal();

  /** Returns the MD5 hash for the list of new features */
  static QString newFeaturesMD5();

  /** Display a generic HTML splash page instead of a message */
  void displaySplashPage( const QString &info );

  /** Display the about page instead of a message */
  void displayAboutPage();

  /** Display the 'please wait' page instead of a message */
  void displayBusyPage();
  /** Display the 'we are currently in offline mode' page instead of a message */
  void displayOfflinePage();

  /** Enable the displaying of messages again after an URL was displayed */
  void enableMsgDisplay();

  /** View message part of type message/RFC822 in extra viewer window. */
  void atmViewMsg(KMMessagePart* msgPart);

  bool atBottom() const;

  bool isFixedFont() { return mUseFixedFont; }
  void setUseFixedFont( bool useFixedFont ) { mUseFixedFont = useFixedFont; }

  /** Return the HtmlWriter connected to the KHTMLPart we use */
  KMail::HtmlWriter * htmlWriter() { return mHtmlWriter; }

  // Action to reply to a message
  // but action( "some_name" ) some name could be used instead.
  KToggleAction *toggleFixFontAction() { return mToggleFixFontAction; }
  KAction *mailToComposeAction() { return mMailToComposeAction; }
  KAction *mailToReplyAction() { return mMailToReplyAction; }
  KAction *mailToForwardAction() { return mMailToForwardAction; }
  KAction *addAddrBookAction() { return mAddAddrBookAction; }
  KAction *openAddrBookAction() { return mOpenAddrBookAction; }
  KAction *copyAction() { return mCopyAction; }
  KAction *selectAllAction() { return mSelectAllAction; }
  KAction *copyURLAction() { return mCopyURLAction; }
  KAction *urlOpenAction() { return mUrlOpenAction; }
  KAction *urlSaveAsAction() { return mUrlSaveAsAction; }
  KAction *addBookmarksAction() { return mAddBookmarksAction;}
  KAction *startImChatAction() { return mStartIMChatAction; }
  // This function returns the complete data that were in this
  // message parts - *after* all encryption has been removed that
  // could be removed.
  // - This is used to store the message in decrypted form.
  void objectTreeToDecryptedMsg( partNode* node,
                                 QByteArray& resultingData,
                                 KMMessage& theMessage,
                                 bool weAreReplacingTheRootNode = false,
                                 int recCount = 0 );

  /** Returns message part from given URL or null if invalid. */
  partNode* partNodeFromUrl(const KUrl &url);

  partNode * partNodeForId( int id );

  /** Returns id of message part from given URL or -1 if invalid. */
  static int msgPartFromUrl(const KUrl &url);

  void setUpdateAttachment( bool update = true ) { mAtmUpdate = update; }

  /** Access to the KHTMLPart used for the viewer. Use with
      care! */
  KHTMLPart * htmlPart() const { return mViewer; }

  /** Returns the current message or 0 if none. */
  KMMessage* message(KMFolder** folder=0) const;

  void openAttachment( int id, const QString & name );

  void emitUrlClicked( const KUrl & url, int button ) {
    emit urlClicked( url, button );
  }

  void emitPopupMenu( const KUrl & url, const QPoint & p ) {
    if ( message() )
      emit popupMenu( *message(), url, p );
  }

  void showAttachmentPopup( int id, const QString & name, const QPoint & p );

  /** Set the serial number of the message this reader window is currently
   *  waiting for. Used to discard updates for already deselected messages. */
  void setWaitingForSerNum( unsigned long serNum ) { mWaitingForSerNum = serNum; }

  QWidget* mainWindow() { return mMainWindow; }

signals:
  /** Emitted after parsing of a message to have it stored
      in unencrypted state in it's folder. */
  void replaceMsgByUnencryptedVersion();

  /** The user presses the right mouse button. 'url' may be 0. */
  void popupMenu(KMMessage &msg, const KUrl &url, const QPoint& mousePos);

  /** The user has clicked onto an URL that is no attachment. */
  void urlClicked(const KUrl &url, int button);

  /** Pgp displays a password dialog */
  void noDrag(void);

public slots:

  /** Select message body. */
  void selectAll();

  /** Force update even if message is the same */
  void clearCache();

  /** Refresh the reader window */
  void updateReaderWin();

  /** HTML Widget scrollbar and layout handling. */
  void slotScrollUp();
  void slotScrollDown();
  void slotScrollPrior();
  void slotScrollNext();
  void slotJumpDown();
  void slotDocumentChanged();
  void slotDocumentDone();
  void slotTextSelected(bool);

  /** An URL has been activate with a click. */
  void slotUrlOpen(const KUrl &url, const KParts::URLArgs &args);

  /** The mouse has moved on or off an URL. */
  void slotUrlOn(const QString &url);

  /** The user presses the right mouse button on an URL. */
  void slotUrlPopup(const QString &, const QPoint& mousePos);

  /** The user selected "Find" from the menu. */
  void slotFind();

  /** The user toggled the "Fixed Font" flag from the view menu. */
  void slotToggleFixedFont();

  /** Copy the selected text to the clipboard */
  void slotCopySelectedText();

   void slotUrlClicked();

  /** Operations on mailto: URLs. */
  void slotMailtoReply();
  void slotMailtoCompose();
  void slotMailtoForward();
  void slotMailtoAddAddrBook();
  void slotMailtoOpenAddrBook();
  /** Copy URL in mUrlCurrent to clipboard. Removes "mailto:" at
      beginning of URL before copying. */
  void slotUrlCopy();
  void slotUrlOpen( const KUrl &url = KUrl() );
  /** Save the page to a file */
  void slotUrlSave();
  void slotAddBookmarks();
  void slotSaveMsg();
  void slotSaveAttachments();

  void slotMessageArrived( KMMessage *msg );
  /** start IM Chat with addressee */
  void slotIMChat();
  void contactStatusChanged( const QString &uid);

	void slotLevelQuote( int l );

protected slots:
  void slotCycleHeaderStyles();
  void slotBriefHeaders();
  void slotFancyHeaders();
  void slotStandardHeaders();
  void slotLongHeaders();
  void slotAllHeaders();

  void slotCycleAttachmentStrategy();
  void slotIconicAttachments();
  void slotSmartAttachments();
  void slotInlineAttachments();
  void slotHideAttachments();

  /** Some attachment operations. */
  void slotAtmView( int id, const QString& name );
  void slotDelayedResize();
  void slotTouchMessage();
  void slotHandleAttachment( int );

protected:
  /** reimplemented in order to update the frame width in case of a changed
      GUI style */
  void styleChange( QStyle& oldStyle );

  /** Set the width of the frame to a reasonable value for the current GUI
      style */
  void setStyleDependantFrameWidth();

  /** Watch for palette changes */
  virtual bool event(QEvent *e);

  /** Calculate the pixel size */
  int pointsToPixel(int pointSize) const;

  /** Feeds the HTML viewer with the contents of the given message.
    HTML begin/end parts are written around the message. */
  void displayMessage();

  /** Parse given message and add it's contents to the reader window. */
  virtual void parseMsg( KMMessage* msg  );

  /** Creates a nice mail header depending on the current selected
    header style. */
  QString writeMsgHeader(KMMessage* aMsg, bool hasVCard=false);

  /** Writes the given message part to a temporary file and returns the
      name of this file or QString() if writing failed.
  */
  QString writeMessagePartToTempFile( KMMessagePart* msgPart, int partNumber );

  /** show window containing infos about a vCard. */
  void showVCard(KMMessagePart *msgPart);

  /** HTML initialization. */
  virtual void initHtmlWidget(void);

  /** Some necessary event handling. */
  virtual void closeEvent(QCloseEvent *);
  virtual void resizeEvent(QResizeEvent *);

  /** Cleanup the attachment temp files */
  virtual void removeTempFiles();

  /** Event filter */
  bool eventFilter( QObject *obj, QEvent *ev );

private slots:
  void slotSetEncoding();

private:
  void adjustLayout();
  void createWidgets();
  void createActions( KActionCollection * ac );
  void saveSplitterSizes( KConfigBase & c ) const;
  QString createAtmFileLink() const;

  KToggleAction * actionForHeaderStyle( const KMail::HeaderStyle *,
                                       const KMail::HeaderStrategy * );
  KToggleAction * actionForAttachmentStrategy( const KMail::AttachmentStrategy * );
  /** Read override codec from configuration */
  void readGlobalOverrideCodec();

private:
  bool mHtmlMail, mHtmlLoadExternal, mHtmlOverride, mHtmlLoadExtOverride;
  int mAtmCurrent;
  QString mAtmCurrentName;
  KMMessage *mMessage;
  // widgets:
  QSplitter * mSplitter;
  KHBox *mBox;
  KMail::HtmlStatusBar *mColorBar;
  KMMimePartTree* mMimePartTree;
  KHTMLPart *mViewer;

  const KMail::AttachmentStrategy * mAttachmentStrategy;
  const KMail::HeaderStrategy * mHeaderStrategy;
  const KMail::HeaderStyle * mHeaderStyle;
  bool mAutoDelete;
  /** where did the user save the attachment last time */
  QString mSaveAttachDir;
  static const int delay;
  QTimer updateReaderWinTimer;
  QTimer mResizeTimer;
  QTimer mDelayedMarkTimer;
  QString mOverrideEncoding;
  QString mOldGlobalOverrideEncoding; // used to detect changes of the global override character encoding
  bool mMsgDisplay;
  bool mNoMDNsWhenEncrypted;
  unsigned long mLastSerNum;
  MessageStatus mLastStatus;

  KMail::CSSHelper * mCSSHelper;
  bool mUseFixedFont;
  bool mPrinting;
  bool mShowColorbar;
  //bool mShowCompleteMessage;
  QStringList mTempFiles;
  QStringList mTempDirs;
  int mMimeTreeMode;
  bool mMimeTreeAtBottom;
  QList<int> mSplitterSizes;
  partNode* mRootNode;
  QString mIdOfLastViewedMessage;
  QWidget *mMainWindow;
  KActionCollection *mActionCollection;
  KAction *mMailToComposeAction, *mMailToReplyAction, *mMailToForwardAction,
      *mAddAddrBookAction, *mOpenAddrBookAction, *mCopyAction, *mCopyURLAction,
      *mUrlOpenAction, *mUrlSaveAsAction, *mAddBookmarksAction, *mStartIMChatAction, *mSelectAllAction;
  KSelectAction *mSelectEncodingAction;
  KToggleAction *mToggleFixFontAction;
  KUrl mUrlClicked;
  KMail::HtmlWriter * mHtmlWriter;
  // an attachment should be updated
  bool mAtmUpdate;
  int mChoice;
  unsigned long mWaitingForSerNum;
  float mSavedRelativePosition;
	int mLevelQuote;
};


#endif

