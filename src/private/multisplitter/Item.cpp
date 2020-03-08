/*
  This file is part of KDDockWidgets.

  Copyright (C) 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "Item_p.h"
#include "MultiSplitterLayout_p.h"
#include "MultiSplitter_p.h"
#include "Logging_p.h"
#include "AnchorGroup_p.h"
#include "Frame_p.h"
#include "DockWidgetBase.h"
#include "Config.h"
#include "FrameworkWidgetFactory.h"

#include <qmath.h>
#include <QEvent>

using namespace KDDockWidgets;

class Item::Private {
public:
    Private(Item *qq, MultiSplitterLayout *layout)
        : q(qq)
        , m_layout(layout)
    {
    }
    MultiSplitterLayout *const m_layout;
    Item *const q;
};


Item::~Item()
{

}

Frame *Item::frame() const
{
    return nullptr;
}

void Item::ref()
{

}

void Item::unref()
{

}
