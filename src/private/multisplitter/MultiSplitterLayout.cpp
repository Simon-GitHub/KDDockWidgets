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
#include "LayoutSaver.h"
#include "AnchorGroup_p.h"

#include <QAction>
#include <QEvent>
#include <QtMath>
#include <QScopedValueRollback>

#define INDICATOR_MINIMUM_LENGTH 100
#define KDDOCKWIDGETS_MIN_WIDTH 80
#define KDDOCKWIDGETS_MIN_HEIGHT 90

using namespace KDDockWidgets;


const ItemList MultiSplitterLayout::items() const
{
    return m_items;
}

QRect MultiSplitterLayout::rectForDrop(const QWidgetOrQuick *widget, Location location, const Item *relativeTo) const
{
    return {};
}

void MultiSplitterLayout::setSize(QSize size)
{
    m_size = size;
}

AnchorGroup MultiSplitterLayout::staticAnchorGroup() const
{
    return {};
}

void MultiSplitterLayout::addWidget(QWidgetOrQuick *widget, Location location, Frame *relativeTo, AddingOption option)
{

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
