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
#include "Convenience_p.h"
#include "AnchorGroup_p.h"

namespace KDDockWidgets {

namespace Debug {
class DebugWindow;
}

class MultiSplitter;


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
    AnchorGroup staticAnchorGroup() const;
    const Anchor::List anchors() const;
    Anchor::List anchors(Qt::Orientation, bool includeStatic = false, bool includePlaceholders = true) const;
    QRect rectForDrop(Length lfd, Location location, QRect relativeToRect) const;
    QRect rectForDrop(const QWidget *widgetBeingDropped, KDDockWidgets::Location location, const Item *relativeTo) const;

    /**
     * @brief Returns the size that the widget will get when dropped at this specific location.
     *
     * When location is Left or Right then the length represents a width, otherwise an height.
     * This function is also called to know the size of the rubberband when hovering over a location.
     */
    Length lengthForDrop(const QWidget *widget, KDDockWidgets::Location location,
                         const Item *relativeTo) const;

    QSize minimumSize() const { return m_minSize; }

    /**
     * @brief Equivalent to @ref availableLengthForOrientation but returns for both orientations.
     * width is for Qt::Vertical.
     */
    QSize availableSize() const;

    QSize size() const { return m_size; }
    void setSize(QSize);

    /**
     * @brief Removes an item from this MultiSplitter.
     */
    void removeItem(Item *item) {} // TODO check if needed

    /**
     * @brief Adds a widget to this MultiSplitter.
     */
    void addWidget(QWidget *widget, KDDockWidgets::Location location, Frame *relativeTo = nullptr, AddingOption option = {});

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

    enum AnchorSanityOption {
        AnchorSanity_Normal = 0,
        AnchorSanity_Intersections = 1,
        AnchorSanity_WidgetMinSizes = 2,
        AnchorSanity_WidgetInvalidSizes = 4,
        AnchorSanity_Followers = 8,
        AnchorSanity_WidgetGeometry = 16,
        AnchorSanity_Visibility = 32,
        AnchorSanity_All = AnchorSanity_Intersections | AnchorSanity_WidgetMinSizes | AnchorSanity_WidgetInvalidSizes | AnchorSanity_Followers | AnchorSanity_WidgetGeometry | AnchorSanity_Visibility
    };
    Q_ENUM(AnchorSanityOption)
    bool checkSanity(AnchorSanityOption o = AnchorSanity_All) { return true; }

    void clear(bool = false);

    bool isEmpty() const { return m_items.isEmpty(); }

    /**
     * @brief returns the contents width.
     * Usually it's the same width as the respective parent MultiSplitter.
     */
    int width() const { return m_size.width(); }

    /**
     * @brief returns the contents height.
     * Usually it's the same height as the respective parent MultiSplitter.
     */
    int height() const { return m_size.height(); }

    /** Returns how much is available for the new drop. It already counts with the space for new anchor that will be created.
     * So it returns this layout's width() (or height), minus the minimum-sizes of all widgets, minus the thickness of all anchors
     * minus the thickness of the anchor that would be created.
     **/
    Length availableLengthForDrop(KDDockWidgets::Location location, const Item *relativeTo) const;

    /**
     * @brief Returns the number of visible Items in this layout.
     * Which is @ref count minus @ref placeholderCount
     * @sa count
     */
    int visibleCount() const;

    /**
     * @brief Returns the number of placeholder items in this layout.
     * This is the same as @ref count minus @ref visibleCount
     * @sa count, visibleCount
     */
    int placeholderCount() const;

    ///@brief returns the number of anchors that are following others, just for tests.
    int numAnchorsFollowing() const;

    ///@brief returns the number of anchors that are following others, just for tests.
    int numVisibleAnchors() const {
        return -1;
    }

    // For debug
    void dumpDebug() const;

    /**
     * @brief Like @ref availableLengthForDrop but just returns the total available width or height (depending on @p orientation)
     * So no need to receive any location.
     * @param orientation If Qt::Vertical then returns the available width. Height otherwise.
     */
    int availableLengthForOrientation(Qt::Orientation orientation) const {
        return -1;
    }

    /**
     * @brief returns @ref contentsWidth if @p o is Qt::Vertical, otherwise @ref contentsHeight
     * @sa contentsHeight, contentsWidth
     */
    int length(Qt::Orientation o) const {
        return -1;
    }

    /**
     * Returns the min or max position that an anchor can go to (due to minimum size restriction on the widgets).
     * For example, if the anchor is vertical and direction is Side1 then it returns the minimum x
     * that the anchor can have. If direction is Side2 then it returns the maximum width. If horizontal
     * then the height.
     */
    int boundPositionForAnchor(Anchor *, Anchor::Side direction) const { return -1; }

    /**
     * Similar to boundPositionForAnchor, but returns both the min and the max width (or height)
     */
    QPair<int, int> boundPositionsForAnchor(Anchor *) const { return {}; }

    /**
     * @brief sets either the contents height if @p o is Qt::Horizontal, otherwise sets the contents width
     */
    void setContentLength(int value, Qt::Orientation o) {}

Q_SIGNALS:
    void visibleWidgetCountChanged();

private:

    // TODO: Review friends
    friend struct AnchorGroup;
    friend class Item;
    friend class Anchor;
    friend class TestDocks;
    friend class KDDockWidgets::Debug::DebugWindow;
    friend class LayoutSaver;

    Item *itemForFrame(const Frame *frame) const;

    QSize m_size;
    QSize m_minSize;
    Anchor::List m_anchors;

    Anchor *m_leftAnchor = nullptr;
    Anchor *m_topAnchor = nullptr;
    Anchor *m_rightAnchor = nullptr;
    Anchor *m_bottomAnchor = nullptr;
    ItemList m_items;
    MultiSplitter *const m_multiSplitter; // TODO: Remove ?
    AnchorGroup m_staticAnchorGroup;
};

}

#endif
