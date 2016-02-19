/*
  metaobjectbrowserwidget.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2010-2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Kevin Funk <kevin.funk@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "metaobjectbrowserwidget.h"
#include "metaobjectbrowserclient.h"
#include "propertywidget.h"

#include <ui/deferredresizemodesetter.h>
#include <ui/deferredtreeviewconfiguration.h>
#include <ui/searchlinecontroller.h>

#include <common/objectbroker.h>
#include <common/tools/metaobjectbrowser/metaobjectbrowserinterface.h>

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QTreeView>

using namespace GammaRay;

static QObject *createMetaObjectBrowserClientHandler(const QString &/*name*/, QObject *parent)
{
  return new MetaObjectBrowserClient(parent);
}

MetaObjectBrowserWidget::MetaObjectBrowserWidget(QWidget *parent)
  : QWidget(parent)
{
  ObjectBroker::registerClientObjectFactoryCallback<MetaObjectBrowserInterface*>(createMetaObjectBrowserClientHandler);


  QAbstractItemModel *model = ObjectBroker::model(QStringLiteral("com.kdab.GammaRay.MetaObjectBrowserTreeModel"));

  m_treeView = new QTreeView(this);
  m_treeView->setIndentation(10);
  m_treeView->setUniformRowHeights(true);
  m_treeView->setModel(model);
  m_treeView->header()->setStretchLastSection(false);
  new DeferredResizeModeSetter(m_treeView->header(), 0, QHeaderView::Stretch);
  new DeferredResizeModeSetter(m_treeView->header(), 1, QHeaderView::ResizeToContents);
  new DeferredResizeModeSetter(m_treeView->header(), 2, QHeaderView::ResizeToContents);
  m_treeView->setSortingEnabled(true);
  m_treeView->setSelectionModel(ObjectBroker::selectionModel(model));
  connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection)));

  auto objectSearchLine = new QLineEdit(this);
  new SearchLineController(objectSearchLine, model);

  PropertyWidget *propertyWidget = new PropertyWidget(this);
  m_propertyWidget = propertyWidget;
  m_propertyWidget->setObjectBaseName(QStringLiteral("com.kdab.GammaRay.MetaObjectBrowser"));

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(objectSearchLine);
  vbox->addWidget(m_treeView);

  QHBoxLayout *hbox = new QHBoxLayout(this);
  hbox->addLayout(vbox);
  hbox->addWidget(propertyWidget);

  // init widget
  new DeferredTreeViewConfiguration(m_treeView);
  m_treeView->sortByColumn(0, Qt::AscendingOrder);

  auto iface = ObjectBroker::object<MetaObjectBrowserInterface*>();
  auto action = new QAction(tr("Scan for issues..."), this);
  connect(action, SIGNAL(triggered()), iface, SLOT(scanForIssues()));
  addAction(action);
}

void MetaObjectBrowserWidget::selectionChanged(const QItemSelection& selection)
{
    if (selection.isEmpty())
        return;

    m_treeView->scrollTo(selection.first().topLeft()); // in case of remote changes
}
