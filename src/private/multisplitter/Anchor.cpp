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

Anchor::Anchor(Qt::Orientation orientation, Anchor::Type type)
    : m_orientation(orientation)
    , m_type(type)
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
