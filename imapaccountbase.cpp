/** -*- c++ -*-
 * imapaccountbase.cpp
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 * Copyright (c) 2002 Marc Mutz <mutz@kde.org>
 *
 * This file is based on work on pop3 and imap account implementations
 * by Don Sanders <sanders@kde.org> and Michael Haeckel <haeckel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "imapaccountbase.h"
using KMail::SieveConfig;

#include "kmacctmgr.h"
#include "kmfolder.h"
#include "kmbroadcaststatus.h"
#include "kmmainwin.h"
#include "kmmessage.h"

#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kio/global.h>
using KIO::MetaData;
#include <kio/passdlg.h>
using KIO::PasswordDialog;
#include <kio/scheduler.h>
//using KIO::Scheduler; // use FQN below

#include <qregexp.h>

namespace KMail {

  static const unsigned short int imapDefaultPort = 143;

  //
  //
  // Ctor and Dtor
  //
  //

  ImapAccountBase::ImapAccountBase( KMAcctMgr * parent, const QString & name )
    : base( parent, name ),
      mPrefix( "/" ),
      mTotal( 0 ),
      mCountUnread( 0 ),
      mCountLastUnread( 0 ),
      mCountRemainChecks( 0 ),
      mAutoExpunge( true ),
      mHiddenFolders( false ),
      mOnlySubscribedFolders( false ),
      mProgressEnabled( false ),
      mIdle( true )
  {
    mPort = imapDefaultPort;
  }

  ImapAccountBase::~ImapAccountBase() {
    kdWarning( mSlave, 5006 )
      << "slave should have been destroyed by subclass!" << endl;
  }

  void ImapAccountBase::init() {
    mPrefix = '/';
    mAutoExpunge = true;
    mHiddenFolders = false;
    mOnlySubscribedFolders = false;
    mProgressEnabled = false;
  }

  void ImapAccountBase::pseudoAssign( const KMAccount * a ) {
    base::pseudoAssign( a );

    const ImapAccountBase * i = dynamic_cast<const ImapAccountBase*>( a );
    if ( !i ) return;

    setPrefix( i->prefix() );
    setAutoExpunge( i->autoExpunge() );
    setHiddenFolders( i->hiddenFolders() );
    setOnlySubscribedFolders( i->onlySubscribedFolders() );
  }

  unsigned short int ImapAccountBase::defaultPort() const {
    return imapDefaultPort;
  }

  QString ImapAccountBase::protocol() const {
    return useSSL() ? "imaps" : "imap";
  }

  //
  //
  // Getters and Setters
  //
  //

  void ImapAccountBase::setPrefix( const QString & prefix ) {
    mPrefix = prefix;
    mPrefix.remove( QRegExp( "[%*\"]" ) );
    if ( mPrefix.isEmpty() || mPrefix[0] != '/' )
      mPrefix.prepend( '/' );
    if ( mPrefix[ mPrefix.length() - 1 ] != '/' )
      mPrefix += '/';
#if 1
    setPrefixHook(); // ### needed while KMFolderCachedImap exists
#else
    if ( mFolder ) mFolder->setImapPath( mPrefix );
#endif
  }

  void ImapAccountBase::setAutoExpunge( bool expunge ) {
    mAutoExpunge = expunge;
  }

  void ImapAccountBase::setHiddenFolders( bool show ) {
    mHiddenFolders = show;
  }

  void ImapAccountBase::setOnlySubscribedFolders( bool show ) {
    mOnlySubscribedFolders = show;
  }

  //
  //
  // read/write config
  //
  //

  void ImapAccountBase::readConfig( /*const*/ KConfig/*Base*/ & config ) {
    base::readConfig( config );

    setPrefix( config.readEntry( "prefix", "/" ) );
    setAutoExpunge( config.readBoolEntry( "auto-expunge", false ) );
    setHiddenFolders( config.readBoolEntry( "hidden-folders", false ) );
    setOnlySubscribedFolders( config.readBoolEntry( "subscribed-folders", false ) );
  }
  
  void ImapAccountBase::writeConfig( KConfig/*Base*/ & config ) /*const*/ {
    base::writeConfig( config );
    
    config.writeEntry( "prefix", prefix() );
    config.writeEntry( "auto-expunge", autoExpunge() );
    config.writeEntry( "hidden-folders", hiddenFolders() );
    config.writeEntry( "subscribed-folders", onlySubscribedFolders() );
  }

  //
  //
  // Network processing
  //
  //

  MetaData ImapAccountBase::slaveConfig() const {
    MetaData m = base::slaveConfig();

    m.insert( "auth", auth() );
    if ( autoExpunge() )
      m.insert( "expunge", "auto" );

    return m;
  }

  bool ImapAccountBase::makeConnection() {
    if ( mSlave ) return true;

    if( mAskAgain || passwd().isEmpty() || login().isEmpty() ) {
      QString log = login();
      QString pass = passwd();
      // We init "store" to true to indicate that we want to have the
      // "keep password" checkbox. Then, we set [Passwords]Keep to
      // storePasswd(), so that the checkbox in the dialog will be
      // init'ed correctly:
      bool store = true;
      KConfigGroup passwords( KGlobal::config(), "Passwords" );
      passwords.writeEntry( "Keep", storePasswd() );
      QString msg = i18n("You need to supply a username and a password to "
			 "access this mailbox.");
      if ( PasswordDialog::getNameAndPassword( log, pass, &store, msg, false,
					       QString::null, name(),
					       i18n("Account:") )
	   != QDialog::Accepted ) {
	emit finishedCheck( false );
	emit newMailsProcessed( 0 ); // taken from kmacctexppop
	return false;
      }
      // The user has been given the chance to change login and
      // password, so copy both from the dialog:
      setPasswd( pass, store );
      setLogin( log );
      mAskAgain = false; // ### taken from kmacctexppop
    }

    mSlave = KIO::Scheduler::getConnectedSlave( getUrl(), slaveConfig() );
    if ( !mSlave ) {
      KMessageBox::error(0, i18n("Could not start process for %1.")
			 .arg( getUrl().protocol() ) );
      return false;
    }

    return true;
  }

  void ImapAccountBase::postProcessNewMail( KMFolder * folder ) {

    disconnect( folder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
		this, SLOT(postProcessNewMail(KMFolder*)) );

    mCountRemainChecks--;

    // count the unread messages
    mCountUnread += folder->countUnread();
    if (mCountRemainChecks == 0) {
      // all checks are done
      if (mCountUnread > 0 && mCountUnread > mCountLastUnread) {
	emit finishedCheck(true);
	mCountLastUnread = mCountUnread;
      } else {
	emit finishedCheck(false);
      }
      mCountUnread = 0;
    }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::initJobData(jobData &jd)
  {
    jd.total = 1;
    jd.done = 0;
    jd.parent = 0;
    jd.quiet = FALSE;
    jd.inboxOnly = FALSE;
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::displayProgress()
  {
    if (mProgressEnabled == mapJobData.isEmpty())
    {
      mProgressEnabled = !mapJobData.isEmpty();
      KMBroadcastStatus::instance()->setStatusProgressEnable( "I" + mName,
          mProgressEnabled );
      if (!mProgressEnabled)
      {
        QPtrListIterator<QGuardedPtr<KMFolder> > it(mOpenFolders);
        for ( it.toFirst() ; it.current() ; ++it )
          if ( it.current() ) (*(it.current()))->close();
        mOpenFolders.clear();
      }
    }
    mIdle = FALSE;
    if (mapJobData.isEmpty())
      mIdleTimer.start(15000);
    else
      mIdleTimer.stop();
    int total = 0, done = 0;
    for (QMap<KIO::Job*, jobData>::Iterator it = mapJobData.begin();
        it != mapJobData.end(); ++it)
    {
      total += (*it).total;
      done += (*it).done;
    }
    if (total == 0)
    {
      mTotal = 0;
      return;
    }
    if (total > mTotal) mTotal = total;
    done += mTotal - total;
    KMBroadcastStatus::instance()->setStatusProgressPercent( "I" + mName,
        100*done / mTotal );
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::listDirectory(QString path, bool onlySubscribed,
      bool secondStep, KMFolder* parent)
  {
    if (!makeConnection()) return;
    // create jobData
    jobData jd;
    jd.total = 1; jd.done = 0;
    jd.inboxOnly = !secondStep && prefix() != "/"
      && path == prefix();
    jd.onlySubscribed = onlySubscribed;
    if (parent) jd.parent = parent;
    if (!secondStep) mHasInbox = FALSE;
    // make the URL
    KURL url = getUrl();
    url.setPath(((jd.inboxOnly) ? QString("/") : path)
        + ";TYPE=" + ((onlySubscribed) ? "LSUB" : "LIST"));
    mSubfolderNames.clear();
    mSubfolderPaths.clear();
    mSubfolderMimeTypes.clear();
    // and go
    KIO::SimpleJob *job = KIO::listDir(url, FALSE);
    KIO::Scheduler::assignJobToSlave(mSlave, job);
    mapJobData.insert(job, jd);
    connect(job, SIGNAL(result(KIO::Job *)),
        this, SLOT(slotListResult(KIO::Job *)));
    connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
        this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
    displayProgress();  
  }  

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds)
  {
    QMap<KIO::Job *, jobData>::Iterator it =
      mapJobData.find(job);
    if (it == mapJobData.end()) return;
    assert(it != mapJobData.end());
    QString name;
    KURL url;
    QString mimeType;
    for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
        udsIt != uds.end(); udsIt++)
    {
      mimeType = QString::null;
      for (KIO::UDSEntry::ConstIterator eIt = (*udsIt).begin();
          eIt != (*udsIt).end(); eIt++)
      {
        // get the needed information
        if ((*eIt).m_uds == KIO::UDS_NAME)
          name = (*eIt).m_str;
        else if ((*eIt).m_uds == KIO::UDS_URL)
          url = KURL((*eIt).m_str, 106); // utf-8
        else if ((*eIt).m_uds == KIO::UDS_MIME_TYPE)
          mimeType = (*eIt).m_str;
      }
      if ((mimeType == "inode/directory" || mimeType == "message/digest"
            || mimeType == "message/directory")
          && name != ".." && (hiddenFolders() || name.at(0) != '.')
          && (!(*it).inboxOnly || name == "INBOX"))
      {
        if (((*it).inboxOnly || 
              url.path() == "/INBOX/") && name == "INBOX")
          mHasInbox = TRUE; // INBOX-only

        // Some servers send _lots_ of duplicates
        if (mSubfolderNames.findIndex(name) == -1)
        {
          mSubfolderNames.append(name);
          mSubfolderPaths.append(url.path());
          mSubfolderMimeTypes.append(mimeType);
        }
      }
    }
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotListResult(KIO::Job * job)
  {
    QMap<KIO::Job *, jobData>::Iterator it =
      mapJobData.find(job);
    if (it == mapJobData.end()) return;
    assert(it != mapJobData.end());
    if (job->error())
    {
      slotSlaveError( mSlave, job->error(),
          job->errorString() );
      if (job->error() == KIO::ERR_SLAVE_DIED) slaveDied();
    }
    if (!job->error())
    {
      // transport the information, include the jobData
      emit receivedFolders(mSubfolderNames, mSubfolderPaths, 
          mSubfolderMimeTypes, *it);
    }
    if (mSlave) mapJobData.remove(it);
    mSubfolderNames.clear();
    mSubfolderPaths.clear();
    mSubfolderMimeTypes.clear();
    displayProgress();
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::changeSubscription( bool subscribe, QString imapPath )
  {
    // change the subscription of the folder
    KURL url = getUrl();
    url.setPath(imapPath);

    QByteArray packedArgs;
    QDataStream stream( packedArgs, IO_WriteOnly);

    if (subscribe)
      stream << (int) 'u' << url;
    else
      stream << (int) 'U' << url;

    // create the KIO-job
    if (!makeConnection()) return;
    KIO::SimpleJob *job = KIO::special(url, packedArgs, FALSE);
    KIO::Scheduler::assignJobToSlave(mSlave, job);
    jobData jd;
    jd.total = 1; jd.done = 0; jd.parent = NULL;
    // a bit of a hack to save one slot
    if (subscribe) jd.onlySubscribed = true;
    else jd.onlySubscribed = false;
    mapJobData.insert(job, jd);

    connect(job, SIGNAL(result(KIO::Job *)),
        SLOT(slotSubscriptionResult(KIO::Job *)));

    displayProgress();
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSubscriptionResult( KIO::Job * job )
  {
    // result of a subscription-job
    QMap<KIO::Job *, jobData>::Iterator it =
      mapJobData.find(job);
    if (it == mapJobData.end()) return;
    if (job->error())
    {
      slotSlaveError( mSlave, job->error(),
          job->errorString() );
      if (job->error() == KIO::ERR_SLAVE_DIED) slaveDied();
    } else {
      emit subscriptionChanged(
          static_cast<KIO::SimpleJob*>(job)->url().path(), (*it).onlySubscribed );
    }
    if (mSlave) mapJobData.remove(it);
    displayProgress();
  }

  //-----------------------------------------------------------------------------
  void ImapAccountBase::slotSlaveError(KIO::Slave *aSlave, int errorCode,
      const QString &errorMsg)
  {
    if (aSlave != mSlave) return;
    if (errorCode == KIO::ERR_SLAVE_DIED) slaveDied();
    if (errorCode == KIO::ERR_COULD_NOT_LOGIN && !mStorePasswd) mAskAgain = TRUE;
    // check if we still display an error
    killAllJobs();
    if ( !mErrorDialogIsActive )
    {
      mErrorDialogIsActive = true;
      if ( KMessageBox::messageBox(kernel->mainWin(), KMessageBox::Error,
            KIO::buildErrorString(errorCode, errorMsg),
            i18n("Error")) == KMessageBox::Ok )
      {
        mErrorDialogIsActive = false;
      }
    } else
      kdDebug(5006) << "suppressing error:" << errorMsg << endl;
  }

  //-----------------------------------------------------------------------------
  QString ImapAccountBase::jobData::htmlURL() const
  {
    KURL u(  url );
    return u.htmlURL();
  }

}; // namespace KMail

#include "imapaccountbase.moc"
