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

/**
 * @file
 * @brief A class to layout widgets in any place relative to another widget.
 *
 * Widgets can be inserted to the left,right,top,bottom in relation to another widget or in relation
 * to the window. Each two neighbour widgets have a separator in between, which the user can use
 * to resize.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_MULTISPLITTER_LAYOUT_P_H
#define KD_MULTISPLITTER_LAYOUT_P_H

#include "Item_p.h"

namespace KDDockWidgets {

namespace Debug {
class DebugWindow;
}

class MultiSplitter;
class AnchorGroup;

/**
 * A MultiSplitter is like a QSplitter but supports mixing vertical and horizontal splitters in
 * any combination.
 *
 * It supports adding a widget to the left/top/bottom/right of the whole MultiSplitter or adding
 * relative to a single widget.
 *
 * A MultiSplitter is simply a list of Anchors, each one of them handling the resizing of widgets.
 * See the documentation for Anchor.
 */
class DOCKS_EXPORT_FOR_UNIT_TESTS MultiSplitterLayout : public QObject // clazy:exclude=ctor-missing-parent-argument
{
    Q_OBJECT

public:

    explicit MultiSplitterLayout(MultiSplitter*);

    const ItemList items() const;
    int count() const { return m_items.size(); }
    const Anchor::List anchors() const { return m_anchors; }
    QRect rectForDrop(const Frame *widget, KDDockWidgets::Location location, const Item *relativeTo) const;
    QSize minimumSize() const { return m_minSize; }
    void setSize(QSize);
    AnchorGroup staticAnchorGroup() const;

    /**
     * @brief Adds a widget to this MultiSplitter.
     */
    void addWidget(Frame *widget, KDDockWidgets::Location location, Frame *relativeTo = nullptr, AddingOption option = {});

    /**
     * Adds an entire MultiSplitter into this layout. The donor MultiSplitter will be deleted
     * after all its Frames are stolen. All added Frames will preserve their original layout, so,
     * if widgetFoo was at the left of widgetBar when in the donor splitter, then it will still be at left
     * of widgetBar when the whole splitter is dropped into this one.
     */
    void addMultiSplitter(MultiSplitter *splitter, KDDockWidgets::Location location,
                          Frame *relativeTo = nullptr);


    /**
     * @brief Adds the dockwidget but it stays hidden until an explicit show()
     */
    void addAsPlaceholder(DockWidgetBase *dw, KDDockWidgets::Location location, Item *relativeTo = nullptr);

    bool checkSanity() { return true; }

    void clear(bool);

Q_SIGNALS:
    void visibleWidgetCountChanged();

private:
    QSize m_size;
    QSize m_minSize;
    Anchor::List m_anchors;

    Anchor *m_leftAnchor = nullptr;
    Anchor *m_topAnchor = nullptr;
    Anchor *m_rightAnchor = nullptr;
    Anchor *m_bottomAnchor = nullptr;
    ItemList m_items;
    MultiSplitter *const m_multiSplitter; // TODO: Remove ?
};

}

#endif
