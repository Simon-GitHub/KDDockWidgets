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

MultiSplitterLayout::MultiSplitterLayout(MultiSplitter *parent)
    : m_multiSplitter(parent)
    , m_leftAnchor(new Anchor(Qt::Vertical, this, Anchor::Type_LeftStatic))
    , m_topAnchor(new Anchor(Qt::Horizontal, this, Anchor::Type_TopStatic))
    , m_rightAnchor(new Anchor(Qt::Vertical, this, Anchor::Type_RightStatic))
    , m_bottomAnchor(new Anchor(Qt::Horizontal, this, Anchor::Type_BottomStatic))
    , m_staticAnchorGroup(this)
{
    Q_ASSERT(parent);
    DockRegistry::self()->registerLayout(this);
    setSize(parent->size());
    qCDebug(multisplittercreation()) << "MultiSplitter";

    connect(this, &MultiSplitterLayout::widgetCountChanged, this, [this] {
        Q_EMIT visibleWidgetCountChanged(visibleCount());
    });

    m_leftAnchor->setObjectName(QStringLiteral("left"));
    m_rightAnchor->setObjectName(QStringLiteral("right"));
    m_bottomAnchor->setObjectName(QStringLiteral("bottom"));
    m_topAnchor->setObjectName(QStringLiteral("top"));

    m_leftAnchor->setFrom(m_topAnchor);
    m_leftAnchor->setTo(m_bottomAnchor);
    m_rightAnchor->setFrom(m_topAnchor);
    m_rightAnchor->setTo(m_bottomAnchor);
    m_topAnchor->setFrom(m_leftAnchor);
    m_topAnchor->setTo(m_rightAnchor);
    m_bottomAnchor->setFrom(m_leftAnchor);
    m_bottomAnchor->setTo(m_rightAnchor);

    m_staticAnchorGroup.left = m_leftAnchor;
    m_staticAnchorGroup.right = m_rightAnchor;
    m_staticAnchorGroup.top = m_topAnchor;
    m_staticAnchorGroup.bottom = m_bottomAnchor;

    clear();
    positionStaticAnchors();
    updateSizeConstraints();
    m_inCtor = false;
}

MultiSplitterLayout::~MultiSplitterLayout()
{
    qCDebug(multisplittercreation) << "~MultiSplitter" << this;
    m_inDestructor = true;
    const auto anchors = m_anchors;
    qDeleteAll(anchors);
    DockRegistry::self()->unregisterLayout(this);
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
    positionStaticAnchors();
}

const AnchorGroup& MultiSplitterLayout::staticAnchorGroup() const
{
    return m_staticAnchorGroup;
}

bool MultiSplitterLayout::validateInputs(QWidgetOrQuick *widget,
                                         Location location,
                                         const Frame *relativeToFrame, AddingOption option) const
{
    if (!widget) {
        qWarning() << Q_FUNC_INFO << "Widget is null";
        return false;
    }

    const bool isDockWidget = qobject_cast<DockWidgetBase*>(widget);
    const bool isStartHidden = option & AddingOption_StartHidden;

    if (!qobject_cast<Frame*>(widget) && !qobject_cast<MultiSplitter*>(widget) && !isDockWidget) {
        qWarning() << "Unknown widget type" << widget;
        return false;
    }

    if (isDockWidget != isStartHidden) {
        qWarning() << "Wrong parameters" << isDockWidget << isStartHidden;
        return false;
    }

    if (relativeToFrame && relativeToFrame == widget) {
        qWarning() << "widget can't be relative to itself";
        return false;
    }

    Item *item = itemForFrame(qobject_cast<Frame*>(widget));

    if (contains(item)) {
        qWarning() << "MultiSplitterLayout::addWidget: Already contains" << widget;
        return false;
    }// TODO: check for widget changing parent

    if (location == Location_None) {
        qWarning() << "MultiSplitterLayout::addWidget: not adding to location None";
        return false;
    }

    const bool relativeToThis = relativeToFrame == nullptr;

    Item *relativeToItem = itemForFrame(relativeToFrame);
    if (!relativeToThis && !contains(relativeToItem)) {
        qWarning() << "MultiSplitterLayout::addWidget: Doesn't contain relativeTo:"
                   << relativeToFrame
                   << "; options=" << option;
        return false;
    }

    return true;
}

void MultiSplitterLayout::addWidget(QWidget *w, Location location, Frame *relativeToWidget, AddingOption option)
{
    // Make some sanity checks:
    if (!validateInputs(w, location, relativeToWidget, option))
        return;

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

void MultiSplitterLayout::clear(bool alsoDeleteStaticAnchors)
{
    const int oldCount = count();
    const int oldVisibleCount = visibleCount();
    const auto items = m_items;
    m_items.clear(); // Clear the item list first, do avoid ~Item() triggering a removal from the list
    qDeleteAll(items);

    const auto anchors = m_anchors;
    m_anchors.clear();

    for (Anchor *anchor : qAsConst(anchors)) {
        anchor->clear();
        if (!anchor->isStatic() || alsoDeleteStaticAnchors) {
            delete anchor;
        }
    }

    if (alsoDeleteStaticAnchors) {
        m_anchors.clear();
        m_topAnchor = nullptr;
        m_bottomAnchor = nullptr;
        m_leftAnchor = nullptr;
        m_rightAnchor = nullptr;
        m_staticAnchorGroup.left = nullptr;
        m_staticAnchorGroup.top = nullptr;
        m_staticAnchorGroup.right = nullptr;
        m_staticAnchorGroup.bottom = nullptr;
    } else {
        m_anchors = { m_topAnchor, m_bottomAnchor, m_leftAnchor, m_rightAnchor };
    }

    if (oldCount > 0)
        Q_EMIT widgetCountChanged(0);
    if (oldVisibleCount > 0)
        Q_EMIT visibleWidgetCountChanged(0);

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
    int count = 0;
    for (auto item : m_items)
        if (!item->isPlaceholder())
            count++;
    return count;
}

int MultiSplitterLayout::placeholderCount() const
{
    return count() - visibleCount();
}

int MultiSplitterLayout::numAnchorsFollowing() const
{
    return -1;
}

int MultiSplitterLayout::numVisibleAnchors() const
{
    int count = 0;
    for (Anchor *a : m_anchors) {
        if (a->separatorWidget()->isVisible())
            count++;
    }

    return count;
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


void MultiSplitterLayout::addItems_internal(const ItemList &items, bool emitSignal)
{
    m_items << items;

    for (auto item : items) {
        item->setLayout(this);
        if (item->frame()) {
            item->setVisible(true);
            item->frame()->installEventFilter(this);
            Q_EMIT widgetAdded(item);
        }
    }

    layoutItems();

    if (emitSignal)
        Q_EMIT widgetCountChanged(m_items.size());
}

void MultiSplitterLayout::layoutItems()
{
    updateSizeConstraints();
    layoutItems_recursive(m_leftAnchor, m_rightAnchor);
    layoutItems_recursive(m_topAnchor, m_bottomAnchor);
    commit();
}

void MultiSplitterLayout::layoutItems_recursive(Anchor *fromAnchor, Anchor *toAnchor)
{
    if (fromAnchor == toAnchor)
        return;

    const Qt::Orientation orientation = fromAnchor->orientation();
    ItemList items = fromAnchor->side2Items();
    for (Item *item : qAsConst(items)) {
        if (item->isPlaceholder())
            continue;

        AnchorGroup &itemAnchors = item->anchorGroup();
        Anchor *oppositeAnchor = itemAnchors.anchorAtSide(Anchor::Side2, orientation);

        const int pos = fromAnchor->position() + fromAnchor->thickness();
        item->setPos(pos, orientation, Anchor::Side1);

        const int minPos2 = pos + item->minLength(orientation) + 1;

        const int pos2 = qMax(qMax(minPos2, item->geometry().right() + 1), oppositeAnchor->position());

        oppositeAnchor->setPosition(pos2);
        layoutItems_recursive(oppositeAnchor, toAnchor);
    }
}

void MultiSplitterLayout::commit()
{
    for (Anchor *anchor : qAsConst(m_anchors))
        anchor->commit();

    for (Item *item : qAsConst(m_items))
        item->commit();
}

void MultiSplitterLayout::positionStaticAnchors()
{
    qCDebug(sizing) << Q_FUNC_INFO;
    m_leftAnchor->setPosition(0);
    m_topAnchor->setPosition(0);
    m_bottomAnchor->setPosition(height() - m_bottomAnchor->thickness());
    m_rightAnchor->setPosition(width() - m_rightAnchor->thickness());

    commit(); // TODO: Commit less. Use state machine.
}

bool MultiSplitterLayout::contains(const Item *item) const
{
    return m_items.contains(const_cast<Item*>(item));
}

bool MultiSplitterLayout::contains(const Frame *frame) const
{
    return itemForFrame(frame) != nullptr;
}

void MultiSplitterLayout::insertAnchor(Anchor *anchor)
{
    m_anchors.append(anchor);
}

void MultiSplitterLayout::removeAnchor(Anchor *anchor)
{
    if (!m_inDestructor)
        m_anchors.removeOne(anchor);
}

QString MultiSplitterLayout::affinityName() const
{
    if (auto ms = multiSplitter()) {
        if (auto mainWindow = ms->mainWindow()) {
            return mainWindow->affinityName();
        } else if (auto fw = ms->floatingWindow()) {
            return fw->affinityName();
        }
    }

    return QString();
}
