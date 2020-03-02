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

/**
 * @file Helper class so dockwidgets can be restored to their previous position.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#include "LastPosition_p.h"
#include "DockRegistry_p.h"
#include "multisplitter/MultiSplitterLayout_p.h"
#include "multisplitter/MultiSplitter_p.h"

#include <algorithm>

using namespace KDDockWidgets;

LastPosition::~LastPosition()
{
    m_placeholders.clear();
}

void LastPosition::addPlaceholderItem(Item *placeholder)
{
    // TODO
}

QWidgetOrQuick *LastPosition::window() const
{
    // TODO
    return nullptr;
}

Item *LastPosition::layoutItem() const
{
    // Return the layout item that is in a MainWindow, that's where we restore the dock widget to.
    // In the future we might want to restore it to FloatingWindows.

    // TODO
    return nullptr;
}

bool LastPosition::containsPlaceholder(Item *item) const
{
    for (const auto &itemRef : m_placeholders)
        if (itemRef->item == item)
            return true;

    return false;
}

void LastPosition::removePlaceholders(const MultiSplitterLayout *layout)
{
 /*   m_placeholders.erase(std::remove_if(m_placeholders.begin(), m_placeholders.end(), [layout] (const std::unique_ptr<ItemRef> &itemref) {
                             return itemref->item->layout() == layout;
                         }), m_placeholders.end());*/
}

void LastPosition::removeNonMainWindowPlaceholders()
{
  /*  auto it = m_placeholders.begin();
    while (it != m_placeholders.end()) {
        ItemRef *itemref = it->get();
        if (!itemref->item->isInMainWindow())
            it = m_placeholders.erase(it);
        else
            ++it;
    }*/
}

void LastPosition::removePlaceholder(Item *placeholder)
{
    if (m_clearing) // reentrancy guard
        return;

    m_placeholders.erase(std::remove_if(m_placeholders.begin(), m_placeholders.end(), [placeholder] (const std::unique_ptr<ItemRef> &itemref) {
                             return itemref->item == placeholder;
    }), m_placeholders.end());
}

void LastPosition::setLastFloatingGeometry(QRect geo)
{
    m_lastFloatingGeo = geo;
}

QRect LastPosition::lastFloatingGeometry() const
{
    return m_lastFloatingGeo;
}

void LastPosition::deserialize(const LayoutSaver::LastPosition &lp)
{
 // TODO

}

LayoutSaver::LastPosition LastPosition::serialize() const
{
// TODO
    return {};
}
