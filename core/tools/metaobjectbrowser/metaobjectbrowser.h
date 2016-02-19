/*
  metaobjectbrowser.h

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

#ifndef GAMMARAY_METAOBJECTBROWSER_METATYPEBROWSER_H
#define GAMMARAY_METAOBJECTBROWSER_METATYPEBROWSER_H

#include "toolfactory.h"

#include <common/tools/metaobjectbrowser/metaobjectbrowserinterface.h>

class QAbstractProxyModel;
class QItemSelection;
class QModelIndex;

namespace GammaRay {

class PropertyController;

class MetaObjectBrowser : public MetaObjectBrowserInterface
{
  Q_OBJECT
  Q_INTERFACES(GammaRay::MetaObjectBrowserInterface)

  public:
    explicit MetaObjectBrowser(ProbeInterface *probe, QObject *parent = 0);

    void scanForIssues() Q_DECL_OVERRIDE;

  private:
    void scanForIssues(const QModelIndex &index);
    void scanForIssues(const QMetaObject *mo);

  private Q_SLOTS:
    void objectSelected(const QItemSelection &selection);
    void objectSelected(QObject *obj);

  private:
     PropertyController *m_propertyController;
     QAbstractProxyModel *m_model;
};

class MetaObjectBrowserFactory : public QObject,
    public StandardToolFactory<QObject, MetaObjectBrowser>
{
  Q_OBJECT
  Q_INTERFACES(GammaRay::ToolFactory)

  public:
    explicit MetaObjectBrowserFactory(QObject *parent) : QObject(parent)
    {
    }

    QString name() const Q_DECL_OVERRIDE;
    QVector<QByteArray> selectableTypes() const Q_DECL_OVERRIDE;
};

}

#endif // GAMMARAY_METAOBJECTBROWSER_METATYPEBROWSER_H
