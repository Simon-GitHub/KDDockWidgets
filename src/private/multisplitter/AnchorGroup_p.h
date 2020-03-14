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

#ifndef KD_MULTISPLITTER_ANCHORGROUP_P_H
#define KD_MULTISPLITTER_ANCHORGROUP_P_H

#include "docks_export.h"
#include "KDDockWidgets.h"
#include "Anchor_p.h"

#include <QDebug>

namespace KDDockWidgets {

class MultiSplitterLayout;
class Anchor;
class Item;

struct DOCKS_EXPORT_FOR_UNIT_TESTS AnchorGroup
{

    explicit AnchorGroup(MultiSplitterLayout *);

    bool isValid() const;
    int width() const;
    int height() const;

    /**
     * @brief Returns the size of an item that would be inside these 4 anchors
     * @sa width(), height()
     */
    QSize size() const;

    Anchor *anchorAtSide(Anchor::Side side, Qt::Orientation orientation) const;
    void setAnchor(Anchor *anchor, Location loc);
    void setAnchor(Anchor *a, Qt::Orientation orientation, Anchor::Side side);
    Anchor *createAnchorFrom(KDDockWidgets::Location fromAnchorLocation, Item *relativeTo);
    Anchor *anchor(KDDockWidgets::Location) const;
    Anchor *oppositeAnchor(Anchor *a) const;
    void addItem(Item *item);
    void addItem(MultiSplitterLayout *);

    bool isStatic() const;

    Anchor *left = nullptr;
    Anchor *top = nullptr;
    Anchor *right = nullptr;
    Anchor *bottom = nullptr;

    QDebug debug(QDebug d) const {
        return d;
    }

    MultiSplitterLayout *m_layout = nullptr;
};

}

inline QDebug operator<< (QDebug d, KDDockWidgets::AnchorGroup *group)
{
    // out-of-line as it needs to include MultiSplitterLayout
    return group->debug(d);
}

#endif
