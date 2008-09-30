/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Till Adam <adam@kde.org>

    KMail is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/


#include "headerlistquicksearch.h"

#include <QApplication>
#include <QEvent>
#include <QLabel>
#include <QTimer>
#include <QToolButton>

#include <kaction.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <k3listview.h>
#include <klocale.h>
#include <kicon.h>
#include "kmfolder.h"
#include "kmheaders.h"
#include "kmsearchpattern.h"
#include "kmmainwidget.h"
#include "kmmessagetag.h"

namespace KMail {

HeaderListQuickSearch::HeaderListQuickSearch( QWidget *parent,
                                              K3ListView *listView )
  : K3ListViewSearchLine( parent, listView ), mStatusCombo(0), mStatus(),  statusList(),
    mComboStatusCount( 0 ), mFilterWithTag( false ), mTagLabel() 

{
  setClickMessage( i18nc("Search for messages.","Search") );

  parent->layout()->addWidget( this );

  QLabel *label = new QLabel( i18n("Stat&us:"), parent );
  label->setObjectName( "quick search status label" );
  parent->layout()->addWidget( label );

  mStatusCombo = new KComboBox( parent );
  mStatusCombo->setObjectName( "quick search status combo box" );
  parent->layout()->addWidget( mStatusCombo );

  updateComboList();

  label->setBuddy( mStatusCombo );
  connect( kmkernel->msgTagMgr(), SIGNAL( msgTagListChanged() ),
           this, SLOT( updateComboList() ) );

  QToolButton *tb = new QToolButton( parent );
  tb->setIcon( KIcon( "edit-find-mail" ) );
  tb->setText( i18n( "Open Full Search" ) );
  tb->setToolTip( tb->text() );
  parent->layout()->addWidget( tb );
  connect( tb, SIGNAL( clicked( bool ) ), SIGNAL( requestFullSearch() ) );

  /* Disable the signal connected by KListViewSearchLine since it will call 
   * itemAdded during KMHeaders::readSortOrder() which will in turn result
   * in getMsgBaseForItem( item ) wanting to access items which are no longer
   * there. Rather rely on KMHeaders::msgAdded and its signal. */
  disconnect(listView, SIGNAL(itemAdded(Q3ListViewItem *)),
             this, SLOT(itemAdded(Q3ListViewItem *)));
  KMHeaders *headers = static_cast<KMHeaders*>( listView );
  connect( headers, SIGNAL( msgAddedToListView( Q3ListViewItem* ) ),
           this, SLOT( itemAdded( Q3ListViewItem* ) ) );

}

HeaderListQuickSearch::~HeaderListQuickSearch()
{
}

bool HeaderListQuickSearch::itemMatches( const Q3ListViewItem *item,
                                         const QString &s ) const
{
  mCurrentSearchTerm = s; // this "hack" used to work in the KDE3 version
  if ( !mStatus.isOfUnknownStatus() ) {
    KMHeaders *headers = static_cast<KMHeaders*>( item->listView() );
    const KMMsgBase *msg = headers->getMsgBaseForItem( item );
    if ( !msg || ! ( msg->messageStatus() & mStatus ) )
      return false;
  } else if ( mFilterWithTag ) {
    KMHeaders *headers = static_cast<KMHeaders*>( item->listView() );
    const KMMsgBase *msg = headers->getMsgBaseForItem( item );
    if ( !msg || !msg->tagList()
              || msg->tagList()->indexOf( mTagLabel ) == -1 )
      return false;
  }
  return K3ListViewSearchLine::itemMatches(item, s);
}

//-----------------------------------------------------------------------------
void HeaderListQuickSearch::reset()
{
  clear();
  mStatusCombo->setCurrentIndex( 0 );
  mFilterWithTag = false;
  slotStatusChanged( 0 );
}

void HeaderListQuickSearch::updateComboList()
{
  disconnect( mStatusCombo, SIGNAL ( activated( int ) ),
           this, SLOT( slotStatusChanged( int ) ) );
  mStatusCombo->clear();
  mStatusCombo->addItem( SmallIcon( "system-run" ), i18n("Any Status") );
  insertStatus( StatusUnread );
  insertStatus( StatusNew );
  insertStatus( StatusImportant );
  insertStatus( StatusReplied );
  insertStatus( StatusForwarded );
  insertStatus( StatusToAct );
  insertStatus( StatusHasAttachment );
  insertStatus( StatusWatched );
  insertStatus( StatusIgnored );

  mComboStatusCount = mStatusCombo->count();
  
  const QList<KMMessageTagDescription *> *msgTagList 
                                        = kmkernel->msgTagMgr()->msgTagList();
  for ( QList<KMMessageTagDescription *>::ConstIterator it = msgTagList->begin(); 
                                              it != msgTagList->end(); ++it ) {
    QString lineString = (*it)->name();
    mStatusCombo->addItem( SmallIcon( (*it)->toolbarIconName() ), lineString );
  }
  mStatusCombo->setCurrentIndex( 0 );
  mFilterWithTag = false;
  connect( mStatusCombo, SIGNAL ( activated( int ) ),
           this, SLOT( slotStatusChanged( int ) ) );
}
void HeaderListQuickSearch::slotStatusChanged( int index )
{
  if ( index == 0 ) {
    mStatus.clear();
    mFilterWithTag = false;
  } else if ( index < mComboStatusCount ) {
    mFilterWithTag = false;
    mStatus = KMSearchRuleStatus::statusFromEnglishName( statusList[index - 1] );
  }
  else {
    mStatus.clear();
    mFilterWithTag = true;
    KMMessageTagDescription *tmp_desc = 
         kmkernel->msgTagMgr()->msgTagList()->at( index - mComboStatusCount );
    mTagLabel = tmp_desc->label();
  }
  updateSearch();
}

void HeaderListQuickSearch::insertStatus(KMail::StatusValueTypes which)
{
  mStatusCombo->addItem( SmallIcon( KMail::StatusValues[which].icon ),
    i18nc( "message status", KMail::StatusValues[ which ].text ) );
  statusList.append( KMail::StatusValues[ which ].text );
}


QString HeaderListQuickSearch::currentSearchTerm() const
{
  return mCurrentSearchTerm;
}


KPIM::MessageStatus HeaderListQuickSearch::currentStatus() const
{
  return mStatus;
}

} // namespace KMail

#include "headerlistquicksearch.moc"
