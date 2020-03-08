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

#include "AnchorGroup_p.h"
#include "Anchor_p.h"
#include "MultiSplitterLayout_p.h"
#include "MultiSplitter_p.h"
#include "Logging_p.h"

using namespace KDDockWidgets;

AnchorGroup::AnchorGroup(MultiSplitterLayout *layout)
    : m_layout(layout)
{
}

bool AnchorGroup::isValid() const
{
    return top && left && bottom && right;
}

int AnchorGroup::width() const
{
    return right->position() - left->position() - left->thickness() - 1;
}

int AnchorGroup::height() const
{
    return bottom->position() - top->position() - bottom->thickness() - 1;
}

QSize AnchorGroup::size() const
{
    return { width(), height() };
}

Anchor *AnchorGroup::anchorAtSide(Anchor::Side side, Qt::Orientation orientation) const
{
    const bool isSide1 = side == Anchor::Side1;
    if (orientation == Qt::Vertical) {
        return isSide1 ? left: right;
    } else {
        return isSide1 ? top : bottom;
    }
}

void AnchorGroup::setAnchor(Anchor *anchor, Location loc)
{
    switch (loc) {
    case KDDockWidgets::Location_OnLeft:
        left = anchor;
        break;
    case KDDockWidgets::Location_OnTop:
        top = anchor;
        break;
    case KDDockWidgets::Location_OnRight:
        right = anchor;
        break;
    case KDDockWidgets::Location_OnBottom:
        bottom = anchor;
        break;
    default:
        Q_ASSERT(false);
    }
}

void AnchorGroup::setAnchor(Anchor *a, Qt::Orientation orientation, Anchor::Side side)
{
    const bool isSide1 = side == Anchor::Side1;
    if (orientation == Qt::Vertical) {
        if (isSide1)
            right = a;
        else
            left = a;
    } else {
        if (isSide1)
            bottom = a;
        else
            top = a;
    }
}

Anchor *AnchorGroup::createAnchorFrom(Location fromAnchorLocation, Item *relativeTo)
{
    Anchor *other = anchor(fromAnchorLocation);
    Q_ASSERT(other);

    auto anchor = new Anchor(other->orientation(), other->m_layout);
    if (anchor->isVertical()) {
        anchor->setFrom(top);
        anchor->setTo(bottom);
    } else {
        anchor->setFrom(left);
        anchor->setTo(right);
    }

    if (relativeTo) {
        if (other->containsItem(relativeTo, Anchor::Side1)) {
            other->removeItem(relativeTo);
            anchor->addItem(relativeTo, Anchor::Side1);
        } else if (other->containsItem(relativeTo, Anchor::Side2)) {
            other->removeItem(relativeTo);
            anchor->addItem(relativeTo, Anchor::Side2);
        } else {
            Q_ASSERT(false);
        }
    } else {
        auto other1 = other->m_side1Items;
        auto other2 = other->m_side2Items;
        other->removeAllItems();
        anchor->addItems(other1, Anchor::Side1);
        anchor->addItems(other2, Anchor::Side2);
    }

    return anchor;
}

Anchor *AnchorGroup::anchor(Location loc) const
{
    switch (loc) {
    case KDDockWidgets::Location_OnLeft:
        return left;
    case KDDockWidgets::Location_OnTop:
        return top;
    case KDDockWidgets::Location_OnRight:
        return right;
    case KDDockWidgets::Location_OnBottom:
        return bottom;
    default:
        Q_ASSERT(false);
        return nullptr;
    }
}

Anchor *AnchorGroup::oppositeAnchor(Anchor *a) const
{
    if (a == left)
        return right;
    if (a == right)
        return left;
    if (a == top)
        return bottom;
    if (a == bottom)
        return top;

    return nullptr;
}

void AnchorGroup::addItem(Item *item)
{
    // Dropping a single dockwidget, without any nesting
    left->addItem(item, Anchor::Side2);
    top->addItem(item, Anchor::Side2);
    right->addItem(item, Anchor::Side1);
    bottom->addItem(item, Anchor::Side1);
}

bool AnchorGroup::isStatic() const
{
    return top->isStatic() && bottom->isStatic() && left->isStatic() && right->isStatic();
}
