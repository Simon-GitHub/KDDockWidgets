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

#include "Anchor_p.h"
#include "MultiSplitterLayout_p.h"
#include "MultiSplitter_p.h"
#include "Logging_p.h"
#include "LayoutSaver.h"
#include "Config.h"
#include "Separator_p.h"
#include "FrameworkWidgetFactory.h"

#include <QRubberBand>
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_WIN
# include <Windows.h>
#endif

using namespace KDDockWidgets;

Anchor::Anchor(Qt::Orientation orientation, MultiSplitterLayout *layout, Anchor::Type type)
    : m_orientation(orientation)
    , m_type(type)
    , m_layout(layout)
    , m_separatorWidget(Config::self().frameworkWidgetFactory()->createSeparator(this, layout->multiSplitter()))
{

}

int Anchor::thickness() const
{
    return 1;
}

bool Anchor::hasItems(Anchor::Side side) const
{
    switch (side) {
    case Side1:
        return !m_side1Items.isEmpty();
    case Side2:
        return !m_side2Items.isEmpty();
    default:
        Q_ASSERT(false);
        return false;
    }
}

int Anchor::thickness(bool staticAnchor)
{
    return Config::self().separatorThickness(staticAnchor);
}

Anchor *Anchor::endFollowee() const
{
    Anchor *a = m_followee;
    while (a) {
        if (!a->followee())
            return a;

        a = a->followee();
    }

    return nullptr;
}

Qt::Orientation Anchor::orientation() const
{
    return m_orientation;
}

int Anchor::position() const
{
    return -1;
}

const ItemList Anchor::items(Anchor::Side side) const
{
    switch (side) {
    case Side1:
        return m_side1Items;
    case Side2:
        return m_side2Items;
    default:
        Q_ASSERT(false);
        return {};
    }
}

bool Anchor::isValid() const
{
    return false;
}

int Anchor::cumulativeMinLength(Anchor::Side side) const
{
    if (isStatic() && isEmpty()) {
        // There's no widget, but minimum is the space occupied by left+right anchors (or top+bottom).
        const int staticAnchorThickness = Anchor::thickness(/*static=*/true);
        if ((side == Side2 && (m_type & (Type_LeftStatic | Type_TopStatic))) ||
            (side == Side1 && (m_type & (Type_RightStatic | Type_BottomStatic))))
            return 2 * staticAnchorThickness;
    }
    const CumulativeMin result = cumulativeMinLength_recursive(side);

    const int numNonStaticAnchors = result.numItems >= 2 ? result.numItems - 1
                                                         : 0;

    const int r = Anchor::thickness(isStatic()) + Anchor::thickness(true)
                  + numNonStaticAnchors*Anchor::thickness(false)
                  + result.minLength;
    return r;
}

Anchor::CumulativeMin Anchor::cumulativeMinLength_recursive(Anchor::Side side) const
{
    const auto items = this->items(side);
    CumulativeMin result = { 0, 0 };

    for (auto item : items) {
        Anchor *oppositeAnchor = item->anchorAtSide(side, orientation());
        if (!oppositeAnchor) {
            // Shouldn't happen. But don't assert as this might be being called from a dumpDebug()
            qWarning() << Q_FUNC_INFO << "Null opposite anchor";
            return {0, 0};
        }

        CumulativeMin candidateMin = { 0, 0 };
        if (!item->isPlaceholder()) {
            candidateMin.numItems++;
            candidateMin.minLength = item->minLength(orientation());
        }

        candidateMin += oppositeAnchor->cumulativeMinLength_recursive(side);

        if (candidateMin.minLength >= result.minLength) {
            result = candidateMin;
        }
    }

    return result;
}

bool Anchor::onlyHasPlaceholderItems(Anchor::Side side) const
{
    auto &items = side == Side1 ? m_side1Items
                                : m_side2Items;

    for (Item *item : items) {
        if (!item->isPlaceholder())
            return false;
    }

    return true;
}

bool Anchor::hasNonPlaceholderItems(Anchor::Side side) const
{
    auto &items = side == Side1 ? m_side1Items
                                : m_side2Items;

    for (Item *item : items) {
        if (!item->isPlaceholder())
            return true;
    }

    return false;
}

bool Anchor::containsItem(const Item *item, Anchor::Side side) const
{
    switch (side) {
    case Side1:
        return m_side1Items.contains(const_cast<Item *>(item));
    case Side2:
        return m_side2Items.contains(const_cast<Item *>(item));
    default:
        Q_ASSERT(false);
        return false;
    }
}

void Anchor::addItem(Item *item, Anchor::Side side)
{
    Q_ASSERT(side != Side_None);
    auto &items = (side == Side1) ? m_side1Items : m_side2Items;
    if (!items.contains(item)) {
        items << item;
        item->anchorGroup().setAnchor(this, orientation(), side);
        Q_EMIT itemsChanged(side);
        //updateItemSizes(); TODO
    }
}

void Anchor::addItems(const ItemList &list, Side side)
{
    for (Item *item : list)
        addItem(item, side);
}

void Anchor::removeAllItems()
{
    removeItems(Side1);
    removeItems(Side2);
}

void Anchor::removeItem(Item *item)
{
    if (m_side1Items.removeOne(item)) {
        item->anchorGroup().setAnchor(nullptr, orientation(), Side1);
        Q_EMIT itemsChanged(Side1);
    } else {
        if (m_side2Items.removeOne(item)) {
            item->anchorGroup().setAnchor(nullptr, orientation(), Side2);
            Q_EMIT itemsChanged(Side2);
        }
    }
}

void Anchor::removeItems(Side side)
{
    const auto &items = this->items(side);
    for (Item *item : items)
        removeItem(item);
}

/** static */
Anchor *Anchor::createFrom(Anchor *other, Item *relativeTo)
{
    Q_ASSERT(other);
    auto anchor = new Anchor(other->orientation(), other->m_layout);
    anchor->setFrom(other->m_from);
    anchor->setTo(other->m_to);

    if (relativeTo) {
        if (other->containsItem(relativeTo, Side1)) {
            other->removeItem(relativeTo);
            anchor->addItem(relativeTo, Side1);
        } else if (other->containsItem(relativeTo, Side2)) {
            other->removeItem(relativeTo);
            anchor->addItem(relativeTo, Side2);
        } else {
            Q_ASSERT(false);
        }
    } else {
        auto other1 = other->m_side1Items;
        auto other2 = other->m_side2Items;
        other->removeAllItems();
        anchor->addItems(other1, Side1);
        anchor->addItems(other2, Side2);
    }

    return anchor;
}


void Anchor::setFrom(Anchor *from)
{
    if (from->orientation() == orientation() || from == this) {
        qWarning() << "Anchor::setFrom: Invalid from" << from->orientation() << m_orientation
                   << from << this;
        return;
    }

    if (m_from)
        disconnect(m_from, &Anchor::positionChanged, this, &Anchor::updateSize);
    m_from = from;
    connect(from, &Anchor::positionChanged, this, &Anchor::updateSize);
    updateSize();

    Q_EMIT fromChanged();
}

void Anchor::setTo(Anchor *to)
{
    Q_ASSERT(to);
    if (to->orientation() == orientation() || to == this) {
        qWarning() << "Anchor::setFrom: Invalid to" << to->orientation() << m_orientation
                   << to << this;
        return;
    }

    if (m_to)
        disconnect(m_to, &Anchor::positionChanged, this, &Anchor::updateSize);
    m_to = to;
    connect(to, &Anchor::positionChanged, this, &Anchor::updateSize);
    updateSize();

    Q_EMIT toChanged();
}

void Anchor::updateSize()
{
    if (isValid()) {
        if (isVertical()) {
            setGeometry(QRect(position(), m_from->geometry().bottom() + 1, thickness(), length()));
        } else {
            setGeometry(QRect(m_from->geometry().right() + 1, position(), length(), thickness()));
        }
    }

    qCDebug(anchors) << "Anchor::updateSize" << this << geometry();
}

int Anchor::length() const
{
    Q_ASSERT(m_to);
    Q_ASSERT(m_from);
    return m_to->position() - m_from->position();
}

void Anchor::setGeometry(QRect r)
{
    if (r != m_geometry) {

        if (position() < 0) {
            qCDebug(anchors) << Q_FUNC_INFO << "Old position was negative" << position() << "; new=" << r;
        }

        m_geometry = r;
        m_separatorWidget->setGeometry(r);
    }
}
