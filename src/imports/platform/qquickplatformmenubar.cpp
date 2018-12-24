/****************************************************************************
**
** Copyright (C) 2019 J-P Nurmi <jpnurmi@gmail.com>
** Copyright (C) 2017 The Qt Company Ltd.
**
** This file is part of QtCellink (based on the Qt Labs Platform module of Qt).
**
** QtCellink is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.

** QtCellink is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.

** You should have received a copy of the GNU General Public License
** along with QtCellink. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "qquickplatformmenubar_p.h"
#include "qquickplatformmenu_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>

/*!
    \qmltype MenuBar
    \inherits QtObject
    \instantiates QQuickPlatformMenuBar
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native menubar.

    The MenuBar type provides a QML API for native platform menubars.

    \image qtlabsplatform-menubar.png

    A menubar consists of a list of drop-down menus.

    \code
    MenuBar {
        id: menuBar

        Menu {
            id: fileMenu
            title: qsTr("File")
            // ...
        }

        Menu {
            id: editMenu
            title: qsTr("&Edit")
            // ...
        }

        Menu {
            id: viewMenu
            title: qsTr("&View")
            // ...
        }

        Menu {
            id: helpMenu
            title: qsTr("&Help")
            // ...
        }
    }
    \endcode

    MenuBar is currently available on the following platforms:

    \list
    \li macOS
    \li Android
    \li Linux (only available on desktop environments that provide a global D-Bus menu bar)
    \endlist

    \labs

    \sa Menu
*/

Q_DECLARE_LOGGING_CATEGORY(qtLabsPlatformMenus)

QQuickPlatformMenuBar::QQuickPlatformMenuBar(QObject *parent)
    : QObject(parent)
{
    m_handle = QGuiApplicationPrivate::platformTheme()->createPlatformMenuBar();
    qCDebug(qtLabsPlatformMenus) << "MenuBar ->" << m_handle;
}

QQuickPlatformMenuBar::~QQuickPlatformMenuBar()
{
    for (QQuickPlatformMenu *menu : qAsConst(m_menus))
        menu->setMenuBar(nullptr);
    delete m_handle;
    m_handle = nullptr;
}

QPlatformMenuBar *QQuickPlatformMenuBar::handle() const
{
    return m_handle;
}

/*!
    \default
    \qmlproperty list<Object> Qt.labs.platform::MenuBar::data

    This default property holds the list of all objects declared as children of
    the menubar. The data property includes objects that are not \l Menu instances,
    such as \l Timer and \l QtObject.

    \sa menus
*/
QQmlListProperty<QObject> QQuickPlatformMenuBar::data()
{
    return QQmlListProperty<QObject>(this, nullptr, data_append, data_count, data_at, data_clear);
}

/*!
    \qmlproperty list<Menu> Qt.labs.platform::MenuBar::menus

    This property holds the list of menus in the menubar.
*/
QQmlListProperty<QQuickPlatformMenu> QQuickPlatformMenuBar::menus()
{
    return QQmlListProperty<QQuickPlatformMenu>(this, nullptr, menus_append, menus_count, menus_at, menus_clear);
}

/*!
    \qmlproperty Window Qt.labs.platform::MenuBar::window

    This property holds the menubar's window.

    Unless explicitly set, the window is automatically resolved by iterating
    the QML parent objects until a \l Window or an \l Item that has a window
    is found.
*/
QWindow *QQuickPlatformMenuBar::window() const
{
    return m_window;
}

void QQuickPlatformMenuBar::setWindow(QWindow *window)
{
    if (m_window == window)
        return;

    if (m_handle)
        m_handle->handleReparent(window);

    m_window = window;
    emit windowChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::addMenu(Menu menu)

    Adds a \a menu to end of the menubar.
*/
void QQuickPlatformMenuBar::addMenu(QQuickPlatformMenu *menu)
{
    insertMenu(m_menus.count(), menu);
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::insertMenu(int index, Menu menu)

    Inserts a \a menu at the specified \a index in the menubar.
*/
void QQuickPlatformMenuBar::insertMenu(int index, QQuickPlatformMenu *menu)
{
    if (!menu || m_menus.contains(menu))
        return;

    QQuickPlatformMenu *before = m_menus.value(index);
    m_menus.insert(index, menu);
    m_data.append(menu);
    menu->setMenuBar(this);
    if (m_handle)
        m_handle->insertMenu(menu->create(), before ? before->handle() : nullptr);
    emit menusChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::removeMenu(Menu menu)

    Removes a \a menu from the menubar.
*/
void QQuickPlatformMenuBar::removeMenu(QQuickPlatformMenu *menu)
{
    if (!menu || !m_menus.removeOne(menu))
        return;

    m_data.removeOne(menu);
    if (m_handle)
        m_handle->removeMenu(menu->handle());
    menu->setMenuBar(nullptr);
    emit menusChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuBar::clear()

    Removes all menus from the menubar.
*/
void QQuickPlatformMenuBar::clear()
{
    if (m_menus.isEmpty())
        return;

    for (QQuickPlatformMenu *menu : qAsConst(m_menus)) {
        m_data.removeOne(menu);
        if (m_handle)
            m_handle->removeMenu(menu->handle());
        menu->setMenuBar(nullptr);
        delete menu;
    }

    m_menus.clear();
    emit menusChanged();
}

void QQuickPlatformMenuBar::classBegin()
{
}

void QQuickPlatformMenuBar::componentComplete()
{
    m_complete = true;
    for (QQuickPlatformMenu *menu : qAsConst(m_menus))
        menu->sync();
    if (!m_window)
        setWindow(findWindow());
}

QWindow *QQuickPlatformMenuBar::findWindow() const
{
    QObject *obj = parent();
    while (obj) {
        QWindow *window = qobject_cast<QWindow *>(obj);
        if (window)
            return window;
        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item && item->window())
            return item->window();
        obj = obj->parent();
    }
    return nullptr;
}

void QQuickPlatformMenuBar::data_append(QQmlListProperty<QObject> *property, QObject *object)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    QQuickPlatformMenu *menu = qobject_cast<QQuickPlatformMenu *>(object);
    if (menu)
        menuBar->addMenu(menu);
    else
        menuBar->m_data.append(object);
}

int QQuickPlatformMenuBar::data_count(QQmlListProperty<QObject> *property)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    return menuBar->m_data.count();
}

QObject *QQuickPlatformMenuBar::data_at(QQmlListProperty<QObject> *property, int index)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    return menuBar->m_data.value(index);
}

void QQuickPlatformMenuBar::data_clear(QQmlListProperty<QObject> *property)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    menuBar->m_data.clear();
}

void QQuickPlatformMenuBar::menus_append(QQmlListProperty<QQuickPlatformMenu> *property, QQuickPlatformMenu *menu)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    menuBar->addMenu(menu);
}

int QQuickPlatformMenuBar::menus_count(QQmlListProperty<QQuickPlatformMenu> *property)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    return menuBar->m_menus.count();
}

QQuickPlatformMenu *QQuickPlatformMenuBar::menus_at(QQmlListProperty<QQuickPlatformMenu> *property, int index)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    return menuBar->m_menus.value(index);
}

void QQuickPlatformMenuBar::menus_clear(QQmlListProperty<QQuickPlatformMenu> *property)
{
    QQuickPlatformMenuBar *menuBar = static_cast<QQuickPlatformMenuBar *>(property->object);
    menuBar->clear();
}
