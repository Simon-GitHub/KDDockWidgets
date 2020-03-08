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

#include "MyMainWindow.h"
#include "MyWidget.h"

//#include <kddockwidgets/LayoutSaver.h>

#include <QMenu>
#include <QMenuBar>
#include <QEvent>
#include <QDebug>
#include <QString>
#include <QTextEdit>

#include <stdlib.h>
#include <time.h>

static MyWidget *newMyWidget()
{
    const int randomNumber = qrand() % 100 + 1;

    if (randomNumber < 50) {
        if (randomNumber < 33) {
            return new MyWidget1();
        } else {
            return new MyWidget3();
        }
    } else {
        return new MyWidget2();
    }
}

MyMainWindow::MyMainWindow(const QString &uniqueName, KDDockWidgets::MainWindowOptions options,
                           bool dockWidget0IsNonClosable, bool restoreIsRelative,
                           const QString &affinityName, QWidget *parent)
    : MainWindow(uniqueName, options, parent)
    , m_dockWidget0IsNonClosable(dockWidget0IsNonClosable)
    , m_restoreIsRelative(restoreIsRelative)
{
    // qApp->installEventFilter(this);

    qsrand(time(nullptr));

    auto menubar = menuBar();
    auto fileMenu = new QMenu(QStringLiteral("File"));
    m_toggleMenu = new QMenu(QStringLiteral("Toggle"));
    menubar->addMenu(fileMenu);
    menubar->addMenu(m_toggleMenu);

    QAction *newAction = fileMenu->addAction(QStringLiteral("New DockWidget"));

    connect(newAction, &QAction::triggered, this, [] {
        static int count = 0;
        count++;
        auto w = newMyWidget();
        w->setGeometry(100, 100, 400, 400);
        auto dock = new KDDockWidgets::DockWidget(QStringLiteral("new dock %1").arg(count));
        dock->setWidget(w);
        dock->resize(600, 600);
        dock->show();
    });

    auto saveLayoutAction = fileMenu->addAction(QStringLiteral("Save Layout"));
    connect(saveLayoutAction, &QAction::triggered, this, [] {
        /*KDDockWidgets::LayoutSaver saver;
        const bool result = saver.saveToFile(QStringLiteral("mylayout.json"));
        qDebug() << "Saving layout to disk. Result=" << result; TODO*/
    });

    auto restoreLayoutAction = fileMenu->addAction(QStringLiteral("Restore Layout"));
    connect(restoreLayoutAction, &QAction::triggered, this, [this] {
       /* KDDockWidgets::RestoreOptions options = KDDockWidgets::RestoreOption_None;
        if (m_restoreIsRelative)
            options |= KDDockWidgets::RestoreOption_RelativeToMainWindow;

        KDDockWidgets::LayoutSaver saver(options);
        saver.restoreFromFile(QStringLiteral("mylayout.json")); TODO */
    });

    auto closeAllAction = fileMenu->addAction(QStringLiteral("Close all"));
    connect(closeAllAction, &QAction::triggered, this, [this] {
        for (auto dw : m_dockwidgets)
            dw->close();
    });

    auto quitAction = fileMenu->addAction(QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    setAffinityName(affinityName);
    createDockWidgets();
}

void MyMainWindow::createDockWidgets()
{
    Q_ASSERT(m_dockwidgets.isEmpty());

    // Create 9 KDDockWidget::DockWidget and the respective widgets they're hosting (MyWidget instances)
    //for (int i = 0; i < 9; i++) TODO
    for (int i = 0; i < 1; i++)
        m_dockwidgets << newDockWidget();


    // MainWindow::addDockWidget() attaches a dock widget to the main window:
    addDockWidget(m_dockwidgets[0], KDDockWidgets::Location_OnTop);

    return; // TODO

    // Here, for finer granularity we specify right of dockwidgets[0]:
    addDockWidget(m_dockwidgets[1], KDDockWidgets::Location_OnRight, m_dockwidgets[0]);

    addDockWidget(m_dockwidgets[2], KDDockWidgets::Location_OnLeft);
    addDockWidget(m_dockwidgets[3], KDDockWidgets::Location_OnBottom);
    addDockWidget(m_dockwidgets[4], KDDockWidgets::Location_OnBottom);

    // Tab two dock widgets toghether
    m_dockwidgets[3]->addDockWidgetAsTab(m_dockwidgets[5]);

    // 6 is floating, as it wasn't added to the main window via MainWindow::addDockWidget().
    // and we tab 7 with it.
    m_dockwidgets[6]->addDockWidgetAsTab(m_dockwidgets[7]);

    // Floating windows also support nesting, here we add 8 to the bottom of the group
    m_dockwidgets[6]->addDockWidgetToContainingWindow(m_dockwidgets[8], KDDockWidgets::Location_OnBottom);
}

KDDockWidgets::DockWidgetBase *MyMainWindow::newDockWidget()
{
    static int count = 0;

    // Passing options is optional, we just want to illustrate Option_NotClosable here
    KDDockWidgets::DockWidget::Options options = KDDockWidgets::DockWidget::Option_None;
    if (count == 0 && m_dockWidget0IsNonClosable)
        options |= KDDockWidgets::DockWidget::Option_NotClosable;

    auto dock = new KDDockWidgets::DockWidget(QStringLiteral("DockWidget #%1").arg(count), options);
    dock->setAffinityName(affinityName()); // optional, just to show the feature. Pass -mi to the example to see incompatible dock widgets

    if (count == 1)
        dock->setIcon(QIcon::fromTheme(QStringLiteral("mail-message")));
    auto myWidget = newMyWidget();
    dock->setWidget(myWidget);
    dock->setTitle(QStringLiteral("DockWidget #%1").arg(count));
    dock->resize(600, 600);
    //dock->show(); TODO
    m_toggleMenu->addAction(dock->toggleAction());

    count++;
    return dock;
}
