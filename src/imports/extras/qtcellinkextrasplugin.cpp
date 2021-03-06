/****************************************************************************
**
** Copyright (C) 2020 CELLINK AB <info@cellink.com>
**
** This file is part of QtCellink.
**
** QtCellink is free software: you can redistribute it and/or modify it
** under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** QtCellink is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with QtCellink. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqml.h>

#include "color.h"
#include "colorimage.h"
#include "filtermodel.h"
#include "iconimage.h"
#include "iconlabel.h"
#include "keyboard.h"
#include "license.h"
#include "licensemodel.h"
#include "mnemoniclabel.h"
#include "navigationgroup.h"
#include "navigationitem.h"
#include "navigationstack.h"
#include "nodedelegate.h"
#include "nodeitem.h"
#include "nodeview.h"
#include "paddedrectangle.h"
#include "rect.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
template <typename T> static void qmlRegisterAnonymousType(const char *, int) { qmlRegisterType<T>(); }
#endif

class QtCellinkExtrasPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    explicit QtCellinkExtrasPlugin(QObject *parent = nullptr);

    void registerTypes(const char *uri) override;
};

QtCellinkExtrasPlugin::QtCellinkExtrasPlugin(QObject *parent)
    : QQmlExtensionPlugin(parent)
{
}

void QtCellinkExtrasPlugin::registerTypes(const char *uri)
{
    qmlRegisterSingletonType<Color>(uri, 1, 0, "Color", [](QQmlEngine *engine, QJSEngine *) -> QObject* { return new Color(engine); });
    qmlRegisterType<ColorImage>(uri, 1, 0, "ColorImage");
    qmlRegisterType<FilterModel>(uri, 1, 0, "FilterModel");
    qmlRegisterType<HeaderDelegate>(uri, 1, 0, "HeaderDelegate");
    qmlRegisterType<IconImage>(uri, 1, 0, "IconImage");
    qmlRegisterType<IconLabel>(uri, 1, 0, "IconLabel");
    qmlRegisterSingletonType<Keyboard>(uri, 1, 0, "Keyboard", [](QQmlEngine *engine, QJSEngine *) -> QObject* { return new Keyboard(engine); });
    qRegisterMetaType<License>();
    qmlRegisterType<LicenseModel>(uri, 1, 0, "LicenseModel");
    qmlRegisterType<MnemonicLabel>(uri, 1, 0, "MnemonicLabel");
    qmlRegisterType<NavigationGroup>(uri, 1, 0, "NavigationGroup");
    qmlRegisterType<NavigationItem>(uri, 1, 0, "NavigationItem");
    qmlRegisterType<NavigationStack>(uri, 1, 0, "NavigationStack");
    qmlRegisterType<NodeItem>(uri, 1, 0, "NodeItem");
    qmlRegisterType<NodeView>(uri, 1, 0, "NodeView");
    qmlRegisterAnonymousType<NodeDelegate>(uri, 1);
    qmlRegisterType<OpacityDelegate>(uri, 1, 0, "OpacityDelegate");
    qmlRegisterType<PaddedRectangle>(uri, 1, 0, "PaddedRectangle");
    qmlRegisterType<ProgressDelegate>(uri, 1, 0, "ProgressDelegate");
    qmlRegisterSingletonType<Rect>(uri, 1, 0, "Rect", [](QQmlEngine *engine, QJSEngine *) -> QObject* { return new Rect(engine); });
    qmlRegisterType<RectDelegate>(uri, 1, 0, "RectDelegate");
    qmlRegisterType<ScaleDelegate>(uri, 1, 0, "ScaleDelegate");
    qmlRegisterType<TextDelegate>(uri, 1, 0, "TextDelegate");

    qmlRegisterRevision<QQuickItem, QT_VERSION_MINOR>(uri, 1, 0);
    qmlRegisterRevision<QQuickFlickable, QT_VERSION_MINOR>(uri, 1, 0);
}

#include "qtcellinkextrasplugin.moc"
