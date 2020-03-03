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

#ifndef KD_MULTISPLITTER_ANCHOR_P_H
#define KD_MULTISPLITTER_ANCHOR_P_H

#include "docks_export.h"
#include "LayoutSaver_p.h"

#include <QObject>
#include <QPointer>
#include <QRect>
#include <QVector>

QT_BEGIN_NAMESPACE
class QRubberBand;
QT_END_NAMESPACE

namespace KDDockWidgets {

class Item;
class MultiSplitterLayout;
class Separator;

typedef QVector<Item*> ItemList;

/**
 * @brief An anchor is the vertical or horizontal (@ref orientation()) line that has an handle
 * so you can resize widgets with your mouse.
 *
 * A MultiSplitter comes with 4 static anchors (@ref isStatic()), that represent the top, left, right
 * and bottom borders. A static anchor means it can't change position, doesn't display the handle and
 * will have the same lifetime has the MultiSplitter.
 *
 * Each anchor has two properties indicating in which anchor it starts and where it ends, @ref from(), to().
 * For example, the top static horizontal anchor starts at the left anchor and ends at the right static anchor.
 * If this anchor is vertical, then from()/to() return horizontal anchors, and vice-versa.
 *
 * An anchor has a length, which is to()->pos() - from()->pos(). The length of a vertical anchor is,
 * thus, its vertical extent (Likewise for horizontal anchors).
 *
 * An anchor controls two groups of widgets: side1 and side2 widgets. When an anchor is dragged with mouse
 * it will resize those widgets. The widgets always start or end at the position where the anchor lives.
 * For vertical anchors, side1 means "the widgets at its left" and side2 means "the widgets at its right",
 * Same principle for horizontal anchors, but for top/bottom instead.
 * Static anchors only have 1 side with widgets. For example the left static anchor only has widgets at its
 * right, so side1Widgets is empty.
 * Non-static anchors, always have side1 and side2 widgets. If not then they are considered unneeded
 * and are deleted.
 *
 * Example:
 *
 * +--------------------+
 * |          |         |
 * |          |         |
 * |          |         |
 * | Foo      |   Bar   |
 * |          |         |
 * |          |         |
 * +--------------------+
 *
 * In the above example we have 5 anchors. 4 of them are static (left, right, top, bottom) and there's
 * a non-static one, in the middle. It's vertical, and can be dragged left and right, resizing its
 * side1Widgets (Foo) and side2Widgets (Bar). This non-static anchors has from=top anchor, and to=bottom anchor.
 *
 */
class DOCKS_EXPORT_FOR_UNIT_TESTS Anchor : public QObject // clazy:exclude=ctor-missing-parent-argument
{
    Q_OBJECT

public:
    typedef QVector<Anchor *> List;

    ///@brief represents the Anchor type
    ///An anchor can be of 2 types:
    /// - Normal: Anchor that can be resized via mouse
    /// - static: this is the top, left, right, bottom borders of the main window. They are called static because they don't move.
    enum Type {
        Type_None = 0, ///< The anchor is normal, and can be resized.
        Type_LeftStatic = 1,   ///< The anchor is static and represents the left mainwindow margin
        Type_RightStatic = 2,  ///< The anchor is static and represents the right mainwindow margin
        Type_TopStatic = 4,    ///< The anchor is static and represents the top mainwindow margin
        Type_BottomStatic = 8, ///< The anchor is static and represents the bottom mainwindow margin
        Type_Static = Type_TopStatic | Type_LeftStatic | Type_RightStatic | Type_BottomStatic ///< The anchor is static, one of the 4 previous ones
    };
    Q_ENUM(Type)

    enum Side {
        Side_None = 0,
        Side1,
        Side2
    };
    Q_ENUM(Side)

    explicit Anchor(Qt::Orientation orientation, Type = Type_None);
    bool isStatic() const { return m_type & Type_Static; }
    bool isFollowing() const { return m_followee != nullptr; }
    int thickness() const;
    bool isVertical() const { return m_orientation == Qt::Vertical; }
    static int thickness(bool staticAnchor);

    /**
     * @brief Returns the last followee in the chain.
     */
    Anchor *endFollowee() const;

    /**
     * @brief getter for the followee
     */
    Anchor *followee() const { return m_followee; }

    Qt::Orientation orientation() const;
    Anchor *from() const { return m_from; }
    Anchor *to() const { return m_to; }

    void setPosition(int) {}
    int position() const;

    /**
     * @brief Checks if this anchor is valid. It's valid if @ref from and @ref to are non-null, and not the same.
     * @return true if this anchor is valid.
     */
    bool isValid() const;

    int cumulativeMinLength(Anchor::Side side) const;

private:
    const Qt::Orientation m_orientation;
    const Type m_type;
    Anchor *m_followee = nullptr;
    QPointer<Anchor> m_from;// QPointer just so we can assert. They should never be null.
    QPointer<Anchor> m_to;

};

}

#endif
