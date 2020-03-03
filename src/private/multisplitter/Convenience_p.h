/*
  This file is part of KDDockWidgets.

  Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "KDDockWidgets.h"

#include <QSize>

#define INDICATOR_MINIMUM_LENGTH 100
#define KDDOCKWIDGETS_MIN_WIDTH 80
#define KDDOCKWIDGETS_MIN_HEIGHT 90

namespace KDDockWidgets {

/**
 * @brief No widget can have a minimum size smaller than this, regardless of their minimum size.
 */
inline QSize hardcodedMinimumSize()
{
    return QSize(KDDOCKWIDGETS_MIN_WIDTH, KDDOCKWIDGETS_MIN_HEIGHT);
}

/**
 * Returns the width of the widget if orientation is Vertical, the height otherwise.
 */
template <typename T>
inline int widgetLength(const T *w, Qt::Orientation orientation)
{
    return (orientation == Qt::Vertical) ? w->width() : w->height();
}

inline int lengthFromSize(QSize sz, Qt::Orientation orientation)
{
    return orientation == Qt::Vertical ? sz.width()
                                       : sz.height();
}

inline Anchor::Side sideForLocation(Location loc)
{
    switch (loc) {
    case KDDockWidgets::Location_OnLeft:
    case KDDockWidgets::Location_OnTop:
        return Anchor::Side1;
    case KDDockWidgets::Location_OnRight:
    case KDDockWidgets::Location_OnBottom:
        return Anchor::Side2;
    default:
        break;
    }

    return Anchor::Side_None;
}

inline Qt::Orientation orientationForLocation(Location loc)
{
    switch (loc) {
    case KDDockWidgets::Location_OnLeft:
    case KDDockWidgets::Location_OnRight:
        return Qt::Vertical;
    case KDDockWidgets::Location_OnTop:
    case KDDockWidgets::Location_OnBottom:
        return Qt::Horizontal;
    default:
        break;
    }

    return Qt::Vertical;
}

inline Qt::Orientation anchorOrientationForLocation(Location l)
{
    return (l == Location_OnLeft || l == Location_OnRight) ? Qt::Vertical
                                                           : Qt::Horizontal;
}

struct Length {
    Length() = default;
    Length(int side1, int side2)
        : side1Length(side1)
        , side2Length(side2)
    {}

    int side1Length = 0;
    int side2Length = 0;
    int length() const { return side1Length + side2Length; }

    void setLength(int newLength)
    {
        // Sets the new length, preserving proportion
        side1Length = int(side1Factor() * newLength);
        side2Length = newLength - side1Length;
    }

    bool isNull() const
    {
        return length() <= 0;
    }

private:
    qreal side1Factor() const
    {
        return (1.0 * side1Length) / length();
    }
};

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

        min = qMax(hardcodedMinimumSize().width(), min);
    } else {
        if (w->minimumHeight() > 0)
            min = w->minimumHeight();
        else
            min = w->minimumSizeHint().height();

        min = qMax(hardcodedMinimumSize().height(), min);
    }

    return qMax(min, 0);
}

}

Q_DECLARE_METATYPE(KDDockWidgets::Length)

