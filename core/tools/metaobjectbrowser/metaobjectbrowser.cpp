/*
  metaobjectbrowser.cpp

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

#include "metaobjectbrowser.h"
#include "metaobjecttreemodel.h"
#include "probe.h"
#include "propertycontroller.h"

#include <common/objectbroker.h>
#include <core/remote/serverproxymodel.h>

#include <3rdparty/kde/krecursivefilterproxymodel.h>

#include <QDebug>
#include <QItemSelectionModel>
#include <QMetaProperty>

using namespace GammaRay;

MetaObjectBrowser::MetaObjectBrowser(ProbeInterface *probe, QObject *parent)
    : MetaObjectBrowserInterface(parent)
    , m_propertyController(new PropertyController(QStringLiteral("com.kdab.GammaRay.MetaObjectBrowser"), this))
{
  m_model = new ServerProxyModel<KRecursiveFilterProxyModel>(this);
  m_model->setSourceModel(Probe::instance()->metaObjectModel());
  probe->registerModel(QStringLiteral("com.kdab.GammaRay.MetaObjectBrowserTreeModel"), m_model);

  QItemSelectionModel *selectionModel = ObjectBroker::selectionModel(m_model);

  connect(selectionModel,SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          SLOT(objectSelected(QItemSelection)));

  m_propertyController->setMetaObject(0); // init

  connect(probe->probe(), SIGNAL(objectSelected(QObject*,QPoint)), this, SLOT(objectSelected(QObject*)));
}

void MetaObjectBrowser::objectSelected(const QItemSelection &selection)
{
  QModelIndex index;
  if (selection.size() == 1)
    index = selection.first().topLeft();

  if (index.isValid()) {
    const QMetaObject *metaObject =
      index.data(MetaObjectTreeModel::MetaObjectRole).value<const QMetaObject*>();
    m_propertyController->setMetaObject(metaObject);
  } else {
    m_propertyController->setMetaObject(0);
  }
}

void MetaObjectBrowser::objectSelected(QObject *obj)
{
    if (!obj)
        return;
    const auto indexes = m_model->match(QModelIndex(), MetaObjectTreeModel::MetaObjectRole, QVariant::fromValue(obj->metaObject()));
    if (indexes.isEmpty())
        return;
    ObjectBroker::selectionModel(m_model)->select(indexes.first(), QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
}

void MetaObjectBrowser::scanForIssues()
{
    scanForIssues(QModelIndex());
}

void MetaObjectBrowser::scanForIssues(const QModelIndex &index)
{
    if (index.isValid()) {
        auto mo = index.data(MetaObjectTreeModel::MetaObjectRole).value<const QMetaObject*>();
        scanForIssues(mo);
    }

    for (int i = 0; i < Probe::instance()->metaObjectModel()->rowCount(index); ++i) {
        const auto childIdx = Probe::instance()->metaObjectModel()->index(i, 0, index);
        if (childIdx.isValid())
            scanForIssues(childIdx);
    }
}

void MetaObjectBrowser::scanForIssues(const QMetaObject* mo)
{
    // TODO output results in a form the client can see...
    Q_ASSERT(mo);
    if (!mo->superClass())
        return;

    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        const auto prop = mo->property(i);

        const auto baseIdx = mo->superClass()->indexOfProperty(prop.name());
        if (baseIdx >= 0)
            qDebug() << mo->className() << prop.name() << "property override";
    }

    for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
        const auto method = mo->method(i);

        const auto baseIdx = mo->superClass()->indexOfMethod(method.methodSignature());
        if (baseIdx >= 0)
            qDebug() << mo->className() << method.methodSignature() << "method override";
    }
}

QString MetaObjectBrowserFactory::name() const
{
  return tr("Meta Objects");
}

QVector<QByteArray> MetaObjectBrowserFactory::selectableTypes() const
{
    return QVector<QByteArray>() << QObject::staticMetaObject.className();
}
