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

    // properties for GammaRay
    Q_PROPERTY(Anchor* from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(Anchor* to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(Anchor *followee READ followee NOTIFY followeeChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation CONSTANT)
    Q_PROPERTY(KDDockWidgets::ItemList side1Items READ side1Items NOTIFY itemsChanged)
    Q_PROPERTY(KDDockWidgets::ItemList side2Items READ side2Items NOTIFY itemsChanged)
    Q_PROPERTY(QString debug_side1ItemNames READ debug_side1ItemNames NOTIFY debug_itemNamesChanged)
    Q_PROPERTY(QString debug_side2ItemNames READ debug_side2ItemNames NOTIFY debug_itemNamesChanged)
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
    enum class SetPositionOption {
        None = 0,
        DontRecalculatePercentage = 1
    };
    Q_DECLARE_FLAGS(SetPositionOptions, SetPositionOption)

    explicit Anchor(Qt::Orientation orientation, MultiSplitterLayout *layout, Type = Type_None);
    ~Anchor();

    bool isStatic() const { return m_type & Type_Static; }
    bool isFollowing() const { return m_followee != nullptr; }
    int thickness() const;
    bool isVertical() const { return m_orientation == Qt::Vertical; }
    bool isEmpty() const { return !hasItems(Side1) && !hasItems(Side2); }
    bool hasItems(Side) const;
    bool hasNonPlaceholderItems(Side) const;
    bool onlyHasPlaceholderItems(Anchor::Side side) const;
    bool containsItem(const Item *w, Side side) const;
    void addItem(Item *, Side);
    void addItems(const ItemList &list, Side);
    void removeItem(Item *w);
    void removeItems(Side);
    void removeAllItems();
    bool isUnneeded() const { return !isStatic() && (!hasItems(Side1) || !hasItems(Side2)); }
    void consume(Anchor *other);
    void consume(Anchor *other, Side);
    void swapItems(Anchor *other);

    /**
     * A squeeze is a widget's width (or height for horizontal anchors) minus its minimum width.
     * This function iterates through all widgets of the specified side and returns the minimum
     * available squeeze.
     */
    int smallestAvailableItemSqueeze(Anchor::Side) const;

    /**
     * Returns how far left or top an anchor can go and still respecting its Side1 widgets min-size.
     * This function doesn't count with shifting other anchors, for that use MultiSplitterLayout::boundPositionsForAnchor()
     * which is is recursive and returns the bounds after simulating that intermediary anchors to the left/top were
     * also resized (each still respecting widgets min sizes though).
     */
    int minPosition() const;

    /**
     * @brief Returns the last followee in the chain.
     */
    Anchor *endFollowee() const;

    ///@brief removes the side1 and side2 items. Doesn't delete them
    void clear();

    /**
     * @brief getter for the followee
     */
    Anchor *followee() const { return m_followee; }

    Qt::Orientation orientation() const;
    void setFrom(Anchor *);
    Anchor *from() const { return m_from; }
    Anchor *to() const { return m_to; }
    void setTo(Anchor *);

    void setPosition(int, Anchor::SetPositionOptions options = SetPositionOption::None);
    int position() const;
    qreal positionPercentage() const { return m_positionPercentage; }

    void commit();

    const ItemList items(Side side) const;
    const ItemList side1Items() const { return m_side1Items; }
    const ItemList side2Items() const { return m_side2Items; }

    /**
     * @brief Checks if this anchor is valid. It's valid if @ref from and @ref to are non-null, and not the same.
     * @return true if this anchor is valid.
     */
    bool isValid() const;

    int cumulativeMinLength(Anchor::Side side) const;

    /**
     * @brief The length of this anchor. The distance between @ref from and @ref to.
     * @return the anchor's length
     */
    int length() const;

    Separator *separatorWidget() const;

    static Anchor *createFrom(Anchor *other, Item *relativeTo = nullptr);
    static int thickness(bool staticAnchor);

Q_SIGNALS:
    void positionChanged(int pos);
    void itemsChanged(Anchor::Side);
    void fromChanged();
    void toChanged();
    void debug_itemNamesChanged();
    void followeeChanged();
    void thicknessChanged();

private:
    void updatePositionPercentage();
    void debug_updateItemNames();
    QString debug_side1ItemNames() const;
    QString debug_side2ItemNames() const;
    void setThickness(); // TODO: check if used
    void updateSize();
    QRect geometry() const { return m_geometry; }
    void setGeometry(QRect);

    friend class AnchorGroup;

    struct CumulativeMin {
        int minLength;
        int numItems;
        CumulativeMin& operator+=(CumulativeMin other) {
            minLength += other.minLength;
            numItems += other.numItems;
            return *this;
        }
    };
    CumulativeMin cumulativeMinLength_recursive(Anchor::Side side) const;

    qreal m_positionPercentage = 0.0; // Should be between 0 and 1
    QRect m_geometry;
    const Qt::Orientation m_orientation;
    ItemList m_side1Items;
    ItemList m_side2Items;
    const Type m_type;
    Anchor *m_followee = nullptr;
    MultiSplitterLayout *m_layout = nullptr;
    Separator *const m_separatorWidget;
    QPointer<Anchor> m_from;// QPointer just so we can assert. They should never be null.
    QPointer<Anchor> m_to;
    QString m_debug_side1ItemNames;
    QString m_debug_side2ItemNames;
};

}

#endif
