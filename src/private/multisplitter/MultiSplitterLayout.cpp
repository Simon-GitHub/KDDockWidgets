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

#define INDICATOR_MINIMUM_LENGTH 100
#define KDDOCKWIDGETS_MIN_WIDTH 80
#define KDDOCKWIDGETS_MIN_HEIGHT 90

using namespace KDDockWidgets;

/**
 * Returns the widget's min-width if orientation is Vertical, the min-height otherwise.
 */
inline int widgetMinLength(const QWidgetOrQuick *w, Qt::Orientation orientation)
{
    int min = 0;
    if (orientation == Qt::Vertical) {
        if (w->minimumWidth() > 0)
            min = w->minimumWidth();
        else
            min = w->minimumSizeHint().width();

        min = qMax(MultiSplitterLayout::hardcodedMinimumSize().width(), min);
    } else {
        if (w->minimumHeight() > 0)
            min = w->minimumHeight();
        else
            min = w->minimumSizeHint().height();

        min = qMax(MultiSplitterLayout::hardcodedMinimumSize().height(), min);
    }

    return qMax(min, 0);
}

MultiSplitterLayout::MultiSplitterLayout(MultiSplitter *ms)
    : m_multiSplitter(ms)
{

}

const ItemList MultiSplitterLayout::items() const
{
    return m_items;
}

MultiSplitterLayout::Length MultiSplitterLayout::availableLengthForDrop(Location location, const Item *relativeTo) const
{
    Length length;

    return length;
}

MultiSplitterLayout::Length MultiSplitterLayout::lengthForDrop(const QWidget *widget, Location location,
                                                               const Item *relativeTo) const
{
    Q_ASSERT(location != Location_None);
    Length available = availableLengthForDrop(location, relativeTo);

    return available;
}

QRect MultiSplitterLayout::rectForDrop(MultiSplitterLayout::Length lfd, Location location, QRect relativeToRect) const
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


QRect MultiSplitterLayout::rectForDrop(const Frame *widgetBeingDropped, Location location, const Item *relativeTo) const
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

AnchorGroup MultiSplitterLayout::staticAnchorGroup() const
{
    return {};
}

void MultiSplitterLayout::addWidget(Frame *w, Location location, Frame *relativeToWidget, AddingOption option)
{
    Anchor *newAnchor = nullptr;
    Item *relativeToItem = itemForFrame(relativeToWidget);
    const QRect dropRect = rectForDrop(w, location, relativeToItem);
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

/**static*/
QSize MultiSplitterLayout::hardcodedMinimumSize()
{
    return QSize(KDDOCKWIDGETS_MIN_WIDTH, KDDOCKWIDGETS_MIN_HEIGHT);
}
