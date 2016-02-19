/*
  metaobjecttreemodel.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2012-2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "metaobjecttreemodel.h"

#include "probe.h"

#include <QDebug>
#include <QThread>
#include <QTimer>

#include <assert.h>

using namespace GammaRay;

namespace GammaRay {
/**
 * Open QObject for access to protected data members
 */
class UnprotectedQObject : public QObject
{
public:
  inline QObjectData* data() const { return d_ptr.data(); }
};
}

/**
 * Return true in case the object has a dynamic meta object
 *
 * If you look at the code generated by moc you'll see this:
 * @code
 * const QMetaObject *GammaRay::MessageModel::metaObject() const
 * {
 *     return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
 * }
 * @endcode
 *
 * QtQuick uses dynamic meta objects (subclasses of QAbstractDynamicMetaObject, defined in qobject_h.p)
 * for QML types. It's possible that these meta objects get destroyed
 * at runtime, so we need to protect against this.
 *
 * @note We cannot say if a specific QMetaObject* is dynamic or not
 * (QMetaObject is non-polymorphic, so we cannot dynamic_cast to QAbstractDynamicMetaObject*),
 * we can just judge by looking at QObjectData of QObject*
 * -- hence the QObject* parameter in hasDynamicMetaObject.
 *
 * @return Return true in case metaObject() does not point to staticMetaObject.
 */
static inline bool hasDynamicMetaObject(const QObject* object)
{
  return reinterpret_cast<const UnprotectedQObject*>(object)->data()->metaObject != 0;
}


MetaObjectTreeModel::MetaObjectTreeModel(Probe *probe)
  : QAbstractItemModel(probe)
  , m_pendingDataChangedTimer(new QTimer(this))
{
  qRegisterMetaType<const QMetaObject *>();

  connect(probe, SIGNAL(objectCreated(QObject*)), this, SLOT(objectAdded(QObject*)));
  // TODO see below
  //connect(probe, SIGNAL(objectDestroyed(QObject*)), this, SLOT(objectRemoved(QObject*)));

  scanMetaTypes();

  m_pendingDataChangedTimer->setInterval(100);
  m_pendingDataChangedTimer->setSingleShot(true);
  connect(m_pendingDataChangedTimer, SIGNAL(timeout()), this, SLOT(emitPendingDataChanged()));
}

MetaObjectTreeModel::~MetaObjectTreeModel()
{
}

QVariant MetaObjectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch (section) {
      case ObjectColumn:
        return tr("Meta Object Class");
      case ObjectSelfCountColumn:
        return tr("Self");
      case ObjectInclusiveCountColumn:
        return tr("Incl.");
      default:
        return QVariant();
    }
  } else if (role == Qt::ToolTipRole) {
    switch (section) {
    case ObjectColumn:
      return tr("This column shows the QMetaObject class hierarchy.");
    case ObjectSelfCountColumn:
      return tr("This column shows the number of objects created of a particular type.");
    case ObjectInclusiveCountColumn:
      return tr("This column shows the number of objects created that inherit from a particular type.");
    default:
      return QVariant();
    }
  }

  return QAbstractItemModel::headerData(section, orientation, role);
}

static bool inheritsQObject(const QMetaObject *mo)
{
    while (mo) {
        if (mo == &QObject::staticMetaObject)
            return true;
        mo = mo->superClass();
    }

    return false;
}

QVariant MetaObjectTreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid()) {
    return QVariant();
  }

  const int column = index.column();
  const QMetaObject *object = metaObjectForIndex(index);
  if (role == Qt::DisplayRole) {
    switch(column) {
      case ObjectColumn:
        return object->className();
      case ObjectSelfCountColumn:
        if (inheritsQObject(object))
          return m_metaObjectInfoMap.value(object).selfCount;
        return QStringLiteral("-");
      case ObjectInclusiveCountColumn:
        if (inheritsQObject(object))
          return m_metaObjectInfoMap.value(object).inclusiveCount;
        return QStringLiteral("-");
      default:
        break;
    }
  } else if (role == MetaObjectRole) {
    return QVariant::fromValue<const QMetaObject*>(object);
  }
  return QVariant();
}

int MetaObjectTreeModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return _Last;
}

int MetaObjectTreeModel::rowCount(const QModelIndex &parent) const
{
  const QMetaObject *metaObject = metaObjectForIndex(parent);
  return m_parentChildMap.value(metaObject).size();
}

QModelIndex MetaObjectTreeModel::parent(const QModelIndex &child) const
{
  if (!child.isValid()) {
    return QModelIndex();
  }

  const QMetaObject *object = metaObjectForIndex(child);
  Q_ASSERT(object);
  const QMetaObject *parentObject = object->superClass();
  return indexForMetaObject(parentObject);
}

QModelIndex MetaObjectTreeModel::index(int row, int column, const QModelIndex &parent) const
{
  const QMetaObject *parentObject = metaObjectForIndex(parent);
  const QVector<const QMetaObject*> children = m_parentChildMap.value(parentObject);
  if (row < 0 || column < 0 || row >= children.size()  || column >= columnCount()) {
    return QModelIndex();
  }

  const QMetaObject *object = children.at(row);
  return createIndex(row, column, const_cast<QMetaObject*>(object));
}

QModelIndexList MetaObjectTreeModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    if (role == MetaObjectRole) {
        const auto mo = value.value<const QMetaObject*>();
        return QModelIndexList() << indexForMetaObject(mo);
    }
    return QAbstractItemModel::match(start, role, value, hits, flags);
}

void MetaObjectTreeModel::objectAdded(QObject *obj)
{
  // Probe::objectFullyConstructed calls us and ensures this already
  Q_ASSERT(thread() == QThread::currentThread());
  Q_ASSERT(Probe::instance()->isValidObject(obj));

  Q_ASSERT(!obj->parent() || Probe::instance()->isValidObject(obj->parent()));

  const QMetaObject *metaObject = obj->metaObject();

  if (hasDynamicMetaObject(obj)) {
    // ideally we would clone the meta object here
    // for now we just move up to the first known static parent meta object, and work with that
    while (metaObject && !isKnownMetaObject(metaObject))
      metaObject = metaObject->superClass();
    if (!metaObject) { // the QML engines actually manages to hit this case, with QObject-ified gadgets...
      qDebug() << "Dangling meta object:" << obj->metaObject()->className();
      return;
    } else {
      qDebug() << "Collapsing dynamic meta object: " << obj->metaObject()->className() << "to" << metaObject->className();
    }
  }

  addMetaObject(metaObject);

  /*
   * This will increase these values:
   * - selfCount for that particular @p metaObject
   * - inclusiveCount for @p metaObject and *all* ancestors
   *
   * Complexity-wise the inclusive count calculation should be okay,
   * since the number of ancestors should be rather small
   * (QMetaObject class hierarchy is rather a broad than a deep tree structure)
   *
   * If this yields some performance issues, we might need to remove the inclusive
   * costs calculation altogether (a calculate-on-request pattern should be even slower)
   */
  ++m_metaObjectInfoMap[metaObject].selfCount;

  // increase inclusive counts
  const QMetaObject* current = metaObject;
  while (current) {
    ++m_metaObjectInfoMap[current].inclusiveCount;
    scheduleDataChange(current);
    current = current->superClass();
  }
}

void MetaObjectTreeModel::scanMetaTypes()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  for (int mtId = 0; mtId <= QMetaType::User || QMetaType::isRegistered(mtId); ++mtId) {
    if (!QMetaType::isRegistered(mtId))
      continue;
    const auto *mt = QMetaType::metaObjectForType(mtId);
    if (mt) {
      addMetaObject(mt);
    }
  }
#endif
  addMetaObject(&staticQtMetaObject);
}

void MetaObjectTreeModel::addMetaObject(const QMetaObject *metaObject)
{
  if (isKnownMetaObject(metaObject)) {
    return;
  }

  const QMetaObject *parentMetaObject = metaObject->superClass();
  if (parentMetaObject && !isKnownMetaObject(parentMetaObject)) {
      // add parent first
      addMetaObject(metaObject->superClass());
  }

  const QModelIndex parentIndex = indexForMetaObject(parentMetaObject);
  // either we get a proper parent and hence valid index or there is no parent
  assert(parentIndex.isValid() || !parentMetaObject);

  QVector<const QMetaObject*> &children = m_parentChildMap[ parentMetaObject ];

  beginInsertRows(parentIndex, children.size(), children.size());
  children.push_back(metaObject);
  m_childParentMap.insert(metaObject, parentMetaObject);

  endInsertRows();
}

void MetaObjectTreeModel::removeMetaObject(const QMetaObject *metaObject)
{
  Q_UNUSED(metaObject);
  // TODO: Meta objects may be deleted, find a way to detect this
}

void MetaObjectTreeModel::objectRemoved(QObject *obj)
{
  Q_ASSERT(thread() == QThread::currentThread());
  Q_UNUSED(obj);
  // TODO

  // decrease counter
  const QMetaObject* metaObject = obj->metaObject();
  const QModelIndex metaModelIndex = indexForMetaObject(metaObject);
  if (!metaModelIndex.isValid()) {
    // something went wrong, ignore
    return;
  }

  assert(m_metaObjectInfoMap.contains(metaObject));
  if (m_metaObjectInfoMap[metaObject].selfCount == 0) {
    // something went wrong, but let's just ignore this event in case of assert
    return;
  }

  --m_metaObjectInfoMap[metaObject].selfCount;
  assert(m_metaObjectInfoMap[metaObject].selfCount >= 0);

  // decrease inclusive counts
  const QMetaObject* current = metaObject;
  while (current) {
    --m_metaObjectInfoMap[current].inclusiveCount;
    assert(m_metaObjectInfoMap[current].inclusiveCount >= 0);
    scheduleDataChange(current);
    current = current->superClass();
  }

  emit dataChanged(metaModelIndex, metaModelIndex);
}

bool MetaObjectTreeModel::isKnownMetaObject(const QMetaObject* metaObject) const
{
  return m_childParentMap.contains(metaObject);
}

QModelIndex MetaObjectTreeModel::indexForMetaObject(const QMetaObject *metaObject) const
{
  if (!metaObject) {
    return QModelIndex();
  }

  const QMetaObject *parentObject = m_childParentMap.value(metaObject);
  Q_ASSERT(parentObject != metaObject);
  const QModelIndex parentIndex = indexForMetaObject(parentObject);
  if (!parentIndex.isValid() && parentObject) {
    return QModelIndex();
  }

  const int row = m_parentChildMap[parentObject].indexOf(metaObject);
  if (row < 0) {
    return QModelIndex();
  }

  return index(row, 0, parentIndex);
}

const QMetaObject *MetaObjectTreeModel::metaObjectForIndex(const QModelIndex &index) const
{
  if (!index.isValid()) {
    return 0;
  }

  void *internalPointer = index.internalPointer();
  const QMetaObject* metaObject = reinterpret_cast<QMetaObject*>(internalPointer);
  return metaObject;
}

void GammaRay::MetaObjectTreeModel::scheduleDataChange(const QMetaObject* mo)
{
    m_pendingDataChanged.insert(mo);
    if (!m_pendingDataChangedTimer->isActive())
        m_pendingDataChangedTimer->start();
}

void GammaRay::MetaObjectTreeModel::emitPendingDataChanged()
{
    foreach (auto mo, m_pendingDataChanged) {
        auto index = indexForMetaObject(mo);
        if (!index.isValid())
            continue;
        emit dataChanged(index.sibling(index.row(), ObjectSelfCountColumn), index.sibling(index.row(), ObjectInclusiveCountColumn));
    }
    m_pendingDataChanged.clear();
}
