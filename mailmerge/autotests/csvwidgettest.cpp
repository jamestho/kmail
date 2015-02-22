/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "csvwidgettest.h"
#include "../widgets/csvwidget.h"
#include <KUrlRequester>
#include <QLabel>
#include <qtest_kde.h>
CsvWidgetTest::CsvWidgetTest(QObject *parent)
    : QObject(parent)
{

}

CsvWidgetTest::~CsvWidgetTest()
{

}

void CsvWidgetTest::shouldHaveDefaultValue()
{
    MailMerge::CsvWidget w;

    QLabel *lab = qFindChild<QLabel *>(&w, QLatin1String("label"));
    QVERIFY(lab);

    KUrlRequester *urlrequester = qFindChild<KUrlRequester *>(&w, QLatin1String("cvsurlrequester"));
    QVERIFY(urlrequester);
}

QTEST_KDEMAIN(CsvWidgetTest, GUI)