/*
  objectinfo.h

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2013-2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Anton Kreuzkamp <anton.kreuzkamp@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  acuordance with GammaRay Commercial License Agreement provided with the Software.

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

#ifndef GAMMARAY_OBJECTINFO_H
#define GAMMARAY_OBJECTINFO_H

#include <QObject>
#include <QDataStream>
#include <QMetaType>
#include <QVector>

namespace GammaRay {

struct ToolInfo
{
  QString id;
  QString name;
};

struct SourceLocation {
    QString filePath;
    uint lineNumber;
    uint columnNumber;
};

struct ObjectInfo {
    explicit ObjectInfo()
    {}
    explicit ObjectInfo(QVector<ToolInfo> &toolInfos, SourceLocation &loc)
        : supportedTools(toolInfos)
        , locationOfInstantiation(loc)
    {}

    QVector<ToolInfo> supportedTools;
    SourceLocation locationOfInstantiation;
};

inline QDataStream &operator<<(QDataStream &out, const ToolInfo &toolInfo)
{
  out << toolInfo.id;
  out << toolInfo.name;
  return out;
}

inline QDataStream &operator>>(QDataStream &in, ToolInfo &toolInfo)
{
  in >> toolInfo.id;
  in >> toolInfo.name;
  return in;
}

inline QDataStream &operator<<(QDataStream &out, const SourceLocation &location)
{
  out << location.filePath;
  out << location.lineNumber;
  out << location.columnNumber;
  return out;
}

inline QDataStream &operator>>(QDataStream &in, SourceLocation &location)
{
  in >> location.filePath;
  in >> location.lineNumber;
  in >> location.columnNumber;
  return in;
}

inline QDataStream &operator<<(QDataStream &out, const ObjectInfo &info)
{
  out << info.supportedTools;
  out << info.locationOfInstantiation;
  return out;
}

inline QDataStream &operator>>(QDataStream &in, ObjectInfo &info)
{
  in >> info.supportedTools;
  in >> info.locationOfInstantiation;
  return in;
}

}

Q_DECLARE_METATYPE(GammaRay::ObjectInfo)

#endif // GAMMARAY_OBJECTINFO_H
