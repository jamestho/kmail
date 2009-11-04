/* KMail Folder Manager
 *
 */
#ifndef kmfoldermgr_h
#define kmfoldermgr_h

#include <QString>

#include <QObject>
#include <QPointer>

#include "kmfolderdir.h"

class KMFolder;

class KMFolderMgr: public QObject
{
  Q_OBJECT

public:
  explicit KMFolderMgr(const QString& basePath, KMFolderDirType dirType = KMStandardDir);
  ~KMFolderMgr();

  /** Returns path to directory where all the folders live. */
  QString basePath() const { return mBasePath; }

  /** Set base path. Also calls reload() on the base directory. */
  void setBasePath(const QString&);


  /** Searches folder and returns it. Skips directories
    (objects of type KMFolderDir) if foldersOnly is true. */
  KMFolder* find(const QString& folderName, bool foldersOnly=true) const;

  /** Searches for a folder with the given id, recurses into directories */
  KMFolder* findIdString(const QString& folderId,
     const uint id = 0, const KMFolderDir *dir = 0) const;

  /** Uses find() to find given folder. If not found the folder is
   * created. Directories are skipped.
   * If an id is passed this searches for it
   */
  KMFolder* findOrCreate(const QString& folderName, bool sysFldr=true,
      const uint id = 0);

  /** Searches folder by id and returns it. Skips directories
    (objects of type KMFolderDir) */
  KMFolder* findById(const uint id) const;

  void        getFolderURLS( QStringList& flist,
                             const QString& prefix=QString(),
                             const KMFolderDir *adir=0 ) const;
  KMFolder*   getFolderByURL( const QString& vpath,
                              const QString& prefix=QString(),
                              const KMFolderDir *adir=0 ) const;

  /** Create a mail folder in the root folder directory dir()
    with given name. Returns Folder on success. */
  KMFolder* createFolder(const QString& fName, bool sysFldr=false,
                         KMFolderType aFolderType=KMFolderTypeMbox,
                         KMFolderDir *aFolderDir = 0);

  /** Physically remove given folder and delete the given folder object. */
  void remove(KMFolder* obsoleteFolder);

  /** emits changed() signal */
  void contentsChanged(void);

  /** Reloads all folders, discarding the existing ones. */
  void reload(void);

  /** Create a list of formatted formatted folder labels and corresponding
   folders*/
  void createFolderList( QStringList *str,
                         QList<QPointer<KMFolder> > *folders );

  /** Auxillary function to facilitate creating a list of formatted
      folder names, suitable for showing in QComboBox */
  void createFolderList( QStringList *str,
                         QList<QPointer<KMFolder> > *folders,
                         KMFolderDir *adir,
                         const QString& prefix,
                         bool i18nized=false );

  /** Create a list of formatted formatted folder labels and corresponding
   folders. The system folder names are translated */
  void createI18nFolderList( QStringList *str,
                             QList<QPointer<KMFolder> > *folders );

  /** fsync all open folders to disk */
  void syncAllFolders( KMFolderDir *adir = 0 );

  /** Compact all folders that need to be, either immediately or scheduled as a background task */
  void compactAllFolders( bool immediate, KMFolderDir *adir = 0 );

  /** Expire old messages in all folders, either immediately or scheduled as a background task */
  void expireAllFolders( bool immediate, KMFolderDir *adir = 0 );

  /** Enable, disable changed() signals */
  void quiet(bool);

  /** Number of folders for purpose of progres report */
  int folderCount(KMFolderDir *dir=0);

  /** Try closing @p folder if possible, something is attempting an exclusive access to it.
      Currently used for KMFolderSearch and the background tasks like expiry */
  void tryReleasingFolder(KMFolder* folder, KMFolderDir *Dir=0);

  /** Create a new unique ID */
  uint createId();

  /** Move a folder */
  void moveFolder( KMFolder* folder, KMFolderDir* newParent );

  /** Rename or move a folder */
  void renameFolder( KMFolder* folder, const QString& newName,
      KMFolderDir* newParent = 0 );

  /** Copy a folder */
  void copyFolder( KMFolder* folder, KMFolderDir* newParent );

  /** Returns the parent Folder for the given folder or 0 on failure.  */
  KMFolder* parentFolder( KMFolder* folder );

public slots:
  /** GUI action: compact all folders that need to be compacted */
  void compactAll() { compactAllFolders( true ); }

  /** GUI action: expire all folders configured as such */
  void expireAll();

  /** Called from KMFolder::remove when the folderstorage was removed */
  void removeFolderAux(KMFolder* obsoleteFolder, bool success);

  /** Called when the renaming of a folder is done */
  void slotRenameDone( const QString &newName, bool success );

signals:
  /** Emitted when the list of folders has changed. This signal is a hook
    where clients like the KMFolderTree tree-view can connect. The signal
    is meant to be emitted whenever the code using the folder-manager
    changed things. */
  void changed();

  /** Emitted, when a folder is about to be removed. */
  void folderRemoved(KMFolder*);

  /** Emitted, when a folder has been added. */
  void folderAdded(KMFolder*);

  /** Emitted, when serial numbers for a folder have been invalidated. */
  void folderInvalidated(KMFolder*);

  /** Emitted, when a message has been appended to a folder */
  void msgAdded(KMFolder*, quint32);

  /** Emitted, when a message has been removed from a folder */
  void msgRemoved(KMFolder*, quint32);

  /** Emitted, when the status of a message is changed */
  void msgChanged(KMFolder*, quint32, int delta);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(KMFolder*, int idx);

  /** Emitted when a folder has been moved or copied */
  void folderMoveOrCopyOperationFinished();

protected:

  /** Auxillary function to facilitate removal of a folder */
  void removeFolder(KMFolder* aFolder);

  /** Auxillary function to facilitate removal of a folder directory */
  void removeDirAux(KMFolderDir* aFolderDir);

  QString mBasePath;
  int mQuiet;
  bool mChanged;
  KMFolder* mRemoveOrig;
};

#endif /*kmfoldermgr_h*/
