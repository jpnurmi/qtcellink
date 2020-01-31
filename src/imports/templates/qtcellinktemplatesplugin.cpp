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
#include <QtQml/qqml.h>

#include "buttonrow.h"
#include "componentview.h"
#include "doublespinbox.h"
#include "flipview.h"
#include "progressindicator.h"
#include "rowbutton.h"
#include "titleseparator.h"

class QtCellinkTemplatesPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtCellinkTemplatesPlugin(QObject *parent = nullptr);
    void registerTypes(const char *uri) override;
};

QtCellinkTemplatesPlugin::QtCellinkTemplatesPlugin(QObject *parent) : QQmlExtensionPlugin(parent)
{
}

void QtCellinkTemplatesPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<ButtonRow>(uri, 1, 0, "ButtonRow");
    qmlRegisterType<ComponentView>(uri, 1, 0, "ComponentView");
    qmlRegisterType<DoubleSpinBox>(uri, 1, 0, "DoubleSpinBox");
    qmlRegisterType<DoubleSpinButton>();
    qmlRegisterType<FlipView>(uri, 1, 0, "FlipView");
    qmlRegisterType<FlipViewAttached>();
    qmlRegisterType<ProgressIndicator>(uri, 1, 0, "ProgressIndicator");
    qmlRegisterType<RowButton>(uri, 1, 0, "RowButton");
    qmlRegisterType<TitleSeparator>(uri, 1, 0, "TitleSeparator");

    qmlRegisterRevision<QQuickItem, QT_VERSION_MINOR>(uri, 1, 0);
    qmlRegisterRevision<QQuickControl, QT_VERSION_MINOR>(uri, 1, 0);
    qmlRegisterRevision<QQuickContainer, QT_VERSION_MINOR>(uri, 1, 0);
    qmlRegisterRevision<QQuickAbstractButton, QT_VERSION_MINOR>(uri, 1, 0);
}

#include "qtcellinktemplatesplugin.moc"
