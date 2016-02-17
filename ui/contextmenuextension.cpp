/*
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

#include "contextmenuextension.h"
#include "clienttoolmodel.h"

#include <common/objectbroker.h>
#include <common/probecontrollerinterface.h>
#include <ui/uiintegration.h>

#include <QMenu>

using namespace GammaRay;

ContextMenuExtension::ContextMenuExtension(ObjectId id)
  : m_id(id)
{
}

void ContextMenuExtension::populateMenu(QMenu *menu)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  if (m_id.isNull())
    return;

  auto probeController = ObjectBroker::object<ProbeControllerInterface*>();
  probeController->requestObjectInfo(m_id);

  // delay adding actions until we know the supported tools
  connect(probeController, &ProbeControllerInterface::objectInfoResponse,
          menu, [menu](ObjectId id, const ObjectInfo &info) {
    foreach (const auto &toolInfo, info.supportedTools) {
      auto action = menu->addAction(QObject::tr("Show in \"%1\" tool").arg(toolInfo.name));
      QObject::connect(action, &QAction::triggered, [id, toolInfo]() {
        auto probeController = ObjectBroker::object<ProbeControllerInterface*>();
        probeController->selectObject(id, toolInfo.id);
      });
    }



    if (UiIntegration::instance() && !info.locationOfInstantiation.filePath.isEmpty()) {
      auto action = menu->addAction(QObject::tr("Show Code: %1:%2:%3")
            .arg(info.locationOfInstantiation.filePath)
            .arg(info.locationOfInstantiation.lineNumber)
            .arg(info.locationOfInstantiation.columnNumber));
      QObject::connect(action, &QAction::triggered, [id, info]() {
        UiIntegration *integ = 0;
            integ = UiIntegration::instance();
            emit integ->navigateToCode(info.locationOfInstantiation.filePath,
                                       info.locationOfInstantiation.lineNumber,
                                       info.locationOfInstantiation.columnNumber);
      });

    }
  });
#endif
}
