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
        Q_ASSERT(m_frame);
        setMinimumSize(frameMinSize());
    }

    QSize frameMinSize() const
    {
        return QSize(widgetMinLength(m_frame, Qt::Vertical),
                     widgetMinLength(m_frame, Qt::Horizontal));
    }

    void setFrame(Frame *frame);
    void setMinimumSize(QSize);
    void updateObjectName();

    Item *const q;
    QRect m_geometry;
    QSize m_minSize;
    QPointer<MultiSplitterLayout> m_layout = nullptr;
    Frame *m_frame = nullptr;
    AnchorGroup m_anchorGroup;
    bool m_destroying = false;
    bool m_isPlaceholder = false;
    int m_refCount = 0;
};

void Item::Private::setMinimumSize(QSize sz)
{
    if (sz != m_minSize) {
        m_minSize = sz;
        Q_EMIT q->minimumSizeChanged();
    }
}

void Item::Private::updateObjectName()
{
    if (m_frame && !m_frame->objectName().isEmpty()) {
        q->setObjectName(m_frame->objectName());
    } else if (q->isPlaceholder()) {
        q->setObjectName(QStringLiteral("placeholder"));
    } else if (!m_frame){
        q->setObjectName(QStringLiteral("null frame"));
    } else {
        q->setObjectName(QStringLiteral("frame with no dockwidgets"));
    }
}

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
    if (!d->m_destroying) {
        d->m_destroying = true;
        //disconnect(d->m_onFrameDestroyed_connection); TODO
        delete d->m_frame;
    }

    if (d->m_layout) {
        d->m_layout->removeItem(this);
    }
    delete d;
}

Frame *Item::frame() const
{
    return d->m_frame;
}

void Item::ref()
{
    d->m_refCount++;
    qCDebug(placeholder()) << Q_FUNC_INFO << "; new ref=" << d->m_refCount;
}

void Item::unref()
{
    if (d->m_refCount == 0) {
        qWarning() << Q_FUNC_INFO << "refcount can't be 0";
        return;
    }

    d->m_refCount--;
    qCDebug(placeholder()) << Q_FUNC_INFO << "; new ref=" << d->m_refCount;

    if (d->m_refCount == 0) {
        if (!d->m_destroying) {
            d->m_destroying = true;
            delete this;
        }
    }
}

int Item::refCount() const
{
    return d->m_refCount;
}

QRect Item::geometry() const
{
    return d->m_geometry;
}

void Item::setGeometry(QRect geo)
{
    Q_ASSERT(d->m_frame || isPlaceholder());
    if (d->m_geometry != geo) {
        d->m_geometry = geo;
        Q_EMIT geometryChanged();
    }
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

QSize Item::size() const
{
    return d->m_geometry.size();
}

void Item::commit() const
{
    if (isPlaceholder())
        return;

    d->m_frame->setGeometry(d->m_geometry);
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

void Item::setPos(int p, Qt::Orientation orientation, Anchor::Side side)
{
    if (orientation == Qt::Vertical) {
        if (side == Anchor::Side1) {
            d->m_geometry.setLeft(p);
        } else {
            d->m_geometry.setRight(p);
        }
    } else {
        if (side == Anchor::Side1) {
            d->m_geometry.setTop(p);
        } else {
            d->m_geometry.setBottom(p);
        }
    }
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

int Item::x() const
{
    return d->m_geometry.x();
}

int Item::y() const
{
    return d->m_geometry.y();
}

QPoint Item::pos() const
{
    return d->m_geometry.topLeft();
}

int Item::position(Qt::Orientation orientation) const
{
    return orientation == Qt::Vertical ? x() : y();
}

QSize Item::minimumSize() const
{
    return isPlaceholder() ? QSize(0, 0)
                           : d->m_minSize;
}

QSize Item::actualMinSize() const
{
    return d->m_minSize;
}
