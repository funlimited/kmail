/*
  progressmanager.cpp

  This file is part of KMail, the KDE mail client.

  Author: Till Adam <adam@kde.org> (C) 2004

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#include <qvaluevector.h>
#include <kdebug.h>

#include "progressmanager.h"

namespace KMail {

KMail::ProgressManager * KMail::ProgressManager::mInstance = 0;
unsigned int KMail::ProgressManager::uID = 42;

ProgressItem::ProgressItem(
       ProgressItem* parent, const QString& id,
       const QString& label, const QString& status, bool canBeCanceled )
       :mId( id ), mLabel( label ), mStatus( status ), mParent( parent ),
        mCanBeCanceled( canBeCanceled ), mProgress( 0 ), mTotal( 0 ),
        mCompleted( 0 ), mWaitingForKids( false ), mCanceled( false )
    {}

ProgressItem::~ProgressItem()
{
}

void ProgressItem::setComplete()
{
   kdDebug(5006) << "ProgressItem::setComplete - " << label() << endl;
   setProgress( 100 );
   if ( mChildren.count() == 0 ) {
     emit progressItemCompleted( this );
     if ( parent() )
       parent()->removeChild( this );
     deleteLater();
   } else {
     mWaitingForKids = true;
   }
}

void ProgressItem::addChild( ProgressItem *kiddo )
{
  mChildren.replace( kiddo, true );
}

void ProgressItem::removeChild( ProgressItem *kiddo )
{
   mChildren.remove( kiddo );
   // in case we were waiting for the last kid to go away, now is the time
   if ( mChildren.count() == 0 && mWaitingForKids ) {
     emit progressItemCompleted( this );
     deleteLater();
   }
}

void ProgressItem::cancel()
{
   if ( mCanceled ) return;
   kdDebug(5006) << "ProgressItem::cancel() - " << label() << endl;
   mCanceled = true;
   // Cancel all children.
   QValueList<ProgressItem*> kids = mChildren.keys();
   QValueList<ProgressItem*>::Iterator it( kids.begin() );
   QValueList<ProgressItem*>::Iterator end( kids.end() );
   for ( ; it != end; it++ ) {
     ProgressItem *kid = *it;
     if ( kid->canBeCanceled() )
       kid->cancel();
   }
   emit progressItemCanceled( this );
}


void ProgressItem::setProgress( unsigned int v )
{
   mProgress = v;
   // kdDebug(5006) << "ProgressItem::setProgress(): " << label() << " : " << v << endl;
   emit progressItemProgress( this, mProgress );
}

void ProgressItem::setStatus( const QString& v )
{
  mStatus = v;
  emit progressItemStatus( this, mStatus );
}

// ======================================

ProgressManager::ProgressManager() :QObject() {
  mInstance = this;
}

ProgressManager::~ProgressManager() { mInstance = 0; }

ProgressItem* ProgressManager::createProgressItemImpl(
     ProgressItem* parent, const QString& id,
     const QString &label, const QString &status, bool cancellable )
{
   ProgressItem *t = 0;
   if ( !mTransactions[ id ] ) {
     t = new ProgressItem ( parent, id, label, status, cancellable );
     mTransactions.insert( id, t );
     if ( parent ) {
       ProgressItem *p = mTransactions[ parent->id() ];
       if ( p ) {
         p->addChild( t );
       }
     }
     // connect all signals
     connect ( t, SIGNAL( progressItemCompleted( ProgressItem* ) ),
               this, SLOT( slotTransactionCompleted( ProgressItem* ) ) );
     connect ( t, SIGNAL( progressItemProgress( ProgressItem*, unsigned int ) ),
               this, SIGNAL( progressItemProgress( ProgressItem*, unsigned int ) ) );
     connect ( t, SIGNAL( progressItemAdded( ProgressItem* ) ),
               this, SIGNAL( progressItemAdded( ProgressItem* ) ) );
     connect ( t, SIGNAL( progressItemCanceled( ProgressItem* ) ),
               this, SIGNAL( progressItemCanceled( ProgressItem* ) ) );
     connect ( t, SIGNAL( progressItemStatus( ProgressItem*, const QString& ) ),
               this, SIGNAL( progressItemStatus( ProgressItem*, const QString& ) ) );

     emit progressItemAdded( t );
   } else {
     // Hm, is this what makes the most sense?
     t = mTransactions[id];
   }
   return t;
}

ProgressItem* ProgressManager::createProgressItemImpl(
     const QString& parent, const QString &id,
     const QString &label, const QString& status,
     bool canBeCanceled )
{
   ProgressItem * p = mTransactions[parent];
   return createProgressItemImpl( p, id, label, status, canBeCanceled );
}


// slots

void ProgressManager::slotTransactionCompleted( ProgressItem *item )
{
   mTransactions.remove( item->id() );
   emit progressItemCompleted( item );
}

void ProgressManager::slotStandardCancelHandler( ProgressItem *item )
{
  item->setComplete();
}

} // namespace
#include "progressmanager.moc"
