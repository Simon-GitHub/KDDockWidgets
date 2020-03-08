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

#include "MultiSplitterLayout_p.h"
#include "Logging_p.h"
#include "MultiSplitter_p.h"
#include "Frame_p.h"
#include "FloatingWindow_p.h"
#include "DockWidgetBase.h"
#include "LastPosition_p.h"
#include "DockRegistry_p.h"
#include "Config.h"
#include "Separator_p.h"
#include "FrameworkWidgetFactory.h"
#include "AnchorGroup_p.h"

#include <QAction>
#include <QEvent>
#include <QtMath>
#include <QScopedValueRollback>

using namespace KDDockWidgets;

MultiSplitterLayout::MultiSplitterLayout(MultiSplitter *ms)
    : m_multiSplitter(ms)
    , m_leftAnchor(new Anchor(Qt::Vertical, this, Anchor::Type_LeftStatic))
    , m_topAnchor(new Anchor(Qt::Horizontal, this, Anchor::Type_TopStatic))
    , m_rightAnchor(new Anchor(Qt::Vertical, this, Anchor::Type_RightStatic))
    , m_bottomAnchor(new Anchor(Qt::Horizontal, this, Anchor::Type_BottomStatic))
    , m_staticAnchorGroup(this)
{
    m_staticAnchorGroup.left = m_leftAnchor;
    m_staticAnchorGroup.right = m_rightAnchor;
    m_staticAnchorGroup.top = m_topAnchor;
    m_staticAnchorGroup.bottom = m_bottomAnchor;

    updateSizeConstraints();
}

const ItemList MultiSplitterLayout::items() const
{
    return m_items;
}

const Anchor::List MultiSplitterLayout::anchors() const { return m_anchors; }

Anchor::List MultiSplitterLayout::anchors(Qt::Orientation orientation, bool includeStatic,
                                          bool includePlaceholders) const
{
    Anchor::List result;
    for (Anchor *anchor : m_anchors) {
        if ((includeStatic || !anchor->isStatic()) && (includePlaceholders || !anchor->isFollowing()) && anchor->orientation() == orientation)
            result << anchor;
    }

    return result;
}

Length MultiSplitterLayout::availableLengthForDrop(Location location, const Item *relativeTo) const
{
    Length length;

    return length;
}

Length MultiSplitterLayout::lengthForDrop(const QWidget *widget, Location location,
                                          const Item *relativeTo) const
{
    Q_ASSERT(location != Location_None);
    Length available = availableLengthForDrop(location, relativeTo);

    return available;
}

QRect MultiSplitterLayout::rectForDrop(Length lfd, Location location, QRect relativeToRect) const
{
    QRect result;
    const int widgetLength = lfd.length();
    const int newAnchorThickness = isEmpty() ? 0 : Anchor::thickness(/*static=*/false);
    const int side1Length = lfd.side1Length;
    const int staticAnchorThickness = Anchor::thickness(/**static=*/true);

    switch (location) {
    case Location_OnLeft:
        result = QRect(qMax(0, relativeToRect.x() - side1Length), relativeToRect.y(),
                       widgetLength, relativeToRect.height());
        break;
    case Location_OnTop:
        result = QRect(relativeToRect.x(), qMax(0, relativeToRect.y() - side1Length),
                       relativeToRect.width(), widgetLength);
        break;
    case Location_OnRight:
        result = QRect(qMin(relativeToRect.right() + 1 - side1Length + newAnchorThickness,
                            width() - widgetLength - staticAnchorThickness), relativeToRect.y(), widgetLength, relativeToRect.height());
        break;
    case Location_OnBottom:
        result = QRect(relativeToRect.x(), qMin(relativeToRect.bottom() + 1 - side1Length + newAnchorThickness,
                                                height() - widgetLength - staticAnchorThickness),
                       relativeToRect.width(), widgetLength);
        break;
    default:
        break;
    }

    qCDebug(sizing) << "MultiSplitterLayout::rectForDrop rect=" << result
                    << "; result.bottomRight=" << result.bottomRight()
                    << "; location=" << location
                    << "; s1=" << side1Length
                    << "; relativeToRect.bottomRight=" << relativeToRect.bottomRight();
    return result;
}


QRect MultiSplitterLayout::rectForDrop(const QWidget *widgetBeingDropped, Location location, const Item *relativeTo) const
{
    Q_ASSERT(widgetBeingDropped);
    Length lfd = lengthForDrop(widgetBeingDropped, location, relativeTo);
    const bool needsMoreSpace = lfd.isNull();
    if (needsMoreSpace)  {
        // This is the case with the drop indicators. If there's not enough space let's still
        // draw some indicator drop. The window will resize to accommodate the drop.
        lfd.side1Length = INDICATOR_MINIMUM_LENGTH / 2;
        lfd.side2Length = INDICATOR_MINIMUM_LENGTH - lfd.side1Length;
    }

    const int staticAnchorThickness = Anchor::thickness(/**static=*/true);
    const bool relativeToThis = relativeTo == nullptr;
    const QRect relativeToRect = relativeToThis ? m_multiSplitter->rect().adjusted(staticAnchorThickness, staticAnchorThickness,
                                                                                  -staticAnchorThickness, -staticAnchorThickness)
                                                : relativeTo->geometry();

    // This function is split in two just so we can unit-test the math in the second one, which is more involved
    QRect result = rectForDrop(lfd, location, relativeToRect);

    return result;
}

void MultiSplitterLayout::setSize(QSize size)
{
    m_size = size;
}

const AnchorGroup& MultiSplitterLayout::staticAnchorGroup() const
{
    return m_staticAnchorGroup;
}

void MultiSplitterLayout::addWidget(QWidget *w, Location location, Frame *relativeToWidget, AddingOption option)
{
    Item *relativeToItem = itemForFrame(relativeToWidget);
    const QRect dropRect = rectForDrop(w, location, relativeToItem);

    auto result = this->createTargetAnchorGroup(location, relativeToItem);
    AnchorGroup targetAnchorGroup = result.first;
    Anchor *newAnchor = result.second;

    auto frame = qobject_cast<Frame*>(w);
    if (frame) {
        auto item = new Item(frame, this);
        targetAnchorGroup.addItem(item);
        addItems_internal(ItemList{ item });
    } else {
        // TODO: Multisplitter drop case
    }


}

void MultiSplitterLayout::addMultiSplitter(MultiSplitter *splitter, Location location, Frame *relativeTo)
{

}

void MultiSplitterLayout::addAsPlaceholder(DockWidgetBase *dw, Location location, Item *relativeTo)
{

}

void MultiSplitterLayout::clear(bool)
{

}

Item *MultiSplitterLayout::itemForFrame(const Frame *frame) const
{
    if (!frame)
        return nullptr;

    for (Item *item : m_items) {
        if (item->frame() == frame)
            return item;
    }
    return nullptr;
}

QPair<AnchorGroup,Anchor*> MultiSplitterLayout::createTargetAnchorGroup(KDDockWidgets::Location location, Item *relativeToItem)
{
    const bool relativeToThis = relativeToItem == nullptr;
    AnchorGroup group = relativeToThis ? staticAnchorGroup()
                                       : anchorsForPos(relativeToItem->geometry().center()); // TODO: By pos seems flaky, better use relativeToItem->anchorGroup() ?

    if (!group.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid anchor group:" << &group
                   << "; staticAnchorGroup=" << &staticAnchorGroup()
                   << "; relativeTo=" << relativeToItem;

        dumpDebug();
    }

    Anchor *newAnchor = nullptr;
    if (relativeToThis) {
        if (!isEmpty())
            newAnchor = this->newAnchor(group, location);
    } else {
        newAnchor = group.createAnchorFrom(location, relativeToItem);
        group.setAnchor(newAnchor, KDDockWidgets::oppositeLocation(location));
    }

    return { group, newAnchor };
}

Anchor *MultiSplitterLayout::newAnchor(AnchorGroup &group, Location location)
{
    qCDebug(::anchors) << "MultiSplitterLayout::newAnchor" << location;
    Anchor *newAnchor = nullptr;
    Anchor *donor = nullptr;
    switch (location) {
    case Location_OnLeft:
        donor = group.left;
        newAnchor = Anchor::createFrom(donor);
        group.right = newAnchor;
        break;
    case Location_OnTop:
        donor = group.top;
        newAnchor = Anchor::createFrom(donor);
        group.bottom = newAnchor;
        break;
    case Location_OnRight:
        donor = group.right;
        newAnchor = Anchor::createFrom(donor);
        group.left = newAnchor;
        break;
    case Location_OnBottom:
        donor = group.bottom;
        newAnchor = Anchor::createFrom(donor);
        group.top = newAnchor;
        break;
    default:
        qWarning() << "MultiSplitterLayout::newAnchor invalid location!";
        return nullptr;
    }

    Q_ASSERT(newAnchor);
    Q_ASSERT(donor);
    Q_ASSERT(donor != newAnchor);

    updateAnchorsFromTo(donor, newAnchor);

    qCDebug(::anchors()) << newAnchor->hasNonPlaceholderItems(Anchor::Side1)
                         << newAnchor->hasNonPlaceholderItems(Anchor::Side2)
                         << newAnchor->side1Items() << newAnchor->side2Items()
                         << "; donor" << donor
                         << "; follows=" << newAnchor->followee();
    return newAnchor;
}

void MultiSplitterLayout::updateSizeConstraints()
{
    const int minH = m_topAnchor->cumulativeMinLength(Anchor::Side2);
    const int minW = m_leftAnchor->cumulativeMinLength(Anchor::Side2);

    const QSize newMinSize = QSize(minW, minH);
    qCDebug(sizing) << Q_FUNC_INFO << "Updating size constraints from" << m_minSize
                    << "to" << newMinSize;

    setMinimumSize(newMinSize);
}

void MultiSplitterLayout::setMinimumSize(QSize sz)
{
    if (sz != m_minSize) {
        m_minSize = sz;
        setSize(m_size.expandedTo(m_minSize)); // Increase size in case we need to
        Q_EMIT minimumSizeChanged(sz);
    }
    qCDebug(sizing) << Q_FUNC_INFO << "minSize = " << m_minSize;
}

int MultiSplitterLayout::visibleCount() const
{
    return -1;
}

int MultiSplitterLayout::placeholderCount() const
{
    return -1;
}

int MultiSplitterLayout::numAnchorsFollowing() const
{
    return -1;
}

void MultiSplitterLayout::dumpDebug() const
{

}

MultiSplitter *MultiSplitterLayout::multiSplitter() const
{
    return m_multiSplitter;
}

QSize MultiSplitterLayout::availableSize() const
{
    return {};
}

Item *MultiSplitterLayout::itemAt(QPoint p) const
{
    for (Item *item : m_items) {
        if (!item->isPlaceholder() && item->geometry().contains(p))
            return item;
    }

    return nullptr;
}

AnchorGroup MultiSplitterLayout::anchorsForPos(QPoint pos) const
{
    Item *item = itemAt(pos);
    if (!item)
        return AnchorGroup(const_cast<MultiSplitterLayout *>(this));

    return item->anchorGroup();
}

void MultiSplitterLayout::updateAnchorsFromTo(Anchor *oldAnchor, Anchor *newAnchor)
{
    // Update the from/to of other anchors
    for (Anchor *other : qAsConst(m_anchors)) {
        Q_ASSERT(other);
        Q_ASSERT(other->isValid());
        if (!other->isStatic() && other->orientation() != newAnchor->orientation()) {
            if (other->to() == oldAnchor) {
                other->setTo(newAnchor);
            } else if (other->from() == oldAnchor) {
                other->setFrom(newAnchor);
            }

            if (!other->isValid()) {
                qDebug() << "MultiSplitterLayout::updateAnchorsFromTo: anchor is now invalid."
                         << "\n    old=" << oldAnchor
                         << "\n    new=" << newAnchor
                         << "\n    from=" << other->from()
                         << "\n    to=" << other->to()
                         << "\n    other=" << other;
            }
        }
    }
}


void MultiSplitterLayout::addItems_internal(const ItemList &items, bool updateConstraints, bool emitSignal)
{
    m_items << items;
    if (updateConstraints)
        updateSizeConstraints();

    for (auto item : items) {
        item->setLayout(this);
        if (item->frame()) {
            item->setVisible(true);
            item->frame()->installEventFilter(this);
            Q_EMIT widgetAdded(item);
        }
    }

    if (emitSignal)
        Q_EMIT widgetCountChanged(m_items.size());
}
