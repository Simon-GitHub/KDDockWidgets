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
    Private(Item *qq, Frame *frame, MultiSplitterLayout *layout)
        : q(qq)
        , m_frame(frame)
        , m_anchorGroup(layout)
    {
    }

    QSize frameMinSize() const
    {
        return QSize(widgetMinLength(m_frame, Qt::Vertical),
                     widgetMinLength(m_frame, Qt::Horizontal));
    }

    void setFrame(Frame *frame);

    Item *const q;
    QRect m_geometry;
    MultiSplitterLayout *m_layout = nullptr;
    Frame *m_frame = nullptr;
    AnchorGroup m_anchorGroup;
};


Item::Item(Frame *frame, MultiSplitterLayout *layout)
    : QObject(layout)
    , d(new Private(this, frame, layout))
{
    setLayout(layout);

    // Minor hack: Set to nullptr so setFrame doesn't bail out. There's a catch-22: setLayout needs to have an m_frame and setFrame needs to have a layout.
    d->m_frame = nullptr;
    d->setFrame(frame);
}

Item::~Item()
{
}

Frame *Item::frame() const
{
    return d->m_frame;
}

void Item::ref()
{
}

void Item::unref()
{
}

int Item::refCount() const
{
    return 0;
}

QRect Item::geometry() const
{
    return d->m_geometry;
}

int Item::length(Qt::Orientation orientation) const
{
    return KDDockWidgets::widgetLength(this, orientation);
}

int Item::minLength(Qt::Orientation orientation) const
{
     return lengthFromSize(minimumSize(), orientation);
}

AnchorGroup &Item::anchorGroup()
{
    return d->m_anchorGroup;
}

const AnchorGroup &Item::anchorGroup() const
{
    return d->m_anchorGroup;
}

int Item::height() const
{
    return size().height();
}

int Item::width() const
{
    return size().width();
}

bool Item::isInMainWindow() const
{
    if (MultiSplitterLayout *l = layout()) {
        if (MultiSplitter *msw = l->multiSplitter()) {
            return msw->isInMainWindow();
        }
    }

    return false;
}

Anchor *Item::anchorAtSide(Anchor::Side side, Qt::Orientation orientation) const
{
    if (!d->m_anchorGroup.isValid())
        qWarning() << Q_FUNC_INFO << "Invalid anchor group" << &d->m_anchorGroup
                   << "in" << this << "; window=" << (parentWidget() ? parentWidget()->window() : nullptr);

    return d->m_anchorGroup.anchorAtSide(side, orientation);
}

QWidget *Item::window() const
{
    Q_ASSERT(d->m_layout);
    Q_ASSERT(d->m_layout->multiSplitter());
    return d->m_layout->multiSplitter()->window();
}

QWidget *Item::parentWidget() const
{
    return d->m_frame ? d->m_frame->parentWidget()
                      : nullptr;
}

MultiSplitterLayout *Item::layout() const
{
    return d->m_layout;
}

bool Item::isVisible() const
{
    Q_ASSERT(d->m_frame);
    return d->m_frame->isVisible();
}

void Item::setVisible(bool v)
{
    Q_ASSERT(d->m_frame);
    d->m_frame->setVisible(v);
}

void Item::setLayout(MultiSplitterLayout *m)
{
    Q_ASSERT(m);
    if (m != d->m_layout) {
        d->m_layout = m;
        d->m_anchorGroup.m_layout = m;
        setParent(m);
        if (d->m_frame)
            d->m_frame->setParent(m->multiSplitter());
    }
}

void Item::Private::setFrame(Frame *frame)
{
    Q_ASSERT((m_frame && !frame) || (!m_frame && frame));

    m_frame = frame;
    Q_EMIT q->frameChanged();
}
