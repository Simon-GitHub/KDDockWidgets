/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

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

#ifndef KD_MULTISPLITTER_ITEM_P_H
#define KD_MULTISPLITTER_ITEM_P_H

#include "docks_export.h"
#include "Anchor_p.h"
#include "QWidgetAdapter.h"
#include "LayoutSaver_p.h"

#include <QRect>
#include <QObject>

/**
 * @brief Implements an item that you put into a multi-splitter.
 * For now it just wraps a KDDockWidgets::Frame, but could eventually be used in QML.
 */
namespace KDDockWidgets {

class Frame;
class AnchorGroup;

class DOCKS_EXPORT_FOR_UNIT_TESTS Item : public QObject // clazy:exclude=ctor-missing-parent-argument
{
    Q_OBJECT
public:

    explicit Item(MultiSplitterLayout *layout);

    /// @brief Destroys its frame too.
    ~Item() override;

    Frame * frame() const; // TODO template
    void ref();
    void unref();
    int refCount() const; // for tests

    bool isPlaceholder() const { return false; }
    QRect geometry() const;
    QSize minimumSize() const { return QSize(); }
    int length(Qt::Orientation) const;
    int minLength(Qt::Orientation orientation) const;

    AnchorGroup& anchorGroup();
    const AnchorGroup& anchorGroup() const;

    int height() const;
    int width() const;
    QSize size() const { return QSize(); }

    bool isInMainWindow() const; // TODO: Make main window agnostic
    QWidget *parentWidget() const; // TODO Check if needed
    void restoreSizes(QSize minSize, QRect geometry) {} // Just for LayoutSaver::restore. // TODO Check if needed
private:
    class Private;
    Private *const d;
};


}

#endif
