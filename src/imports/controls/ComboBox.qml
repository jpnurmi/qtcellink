/****************************************************************************
**
** Copyright (C) 2019 CELLINK AB <info@cellink.com>
** Copyright (C) 2017 The Qt Company Ltd.
**
** This file is part of QtCellink (based on the Qt Quick Controls 2 module of Qt).
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

import QtQuick 2.11
import QtQuick.Window 2.3
import QtQuick.Templates 2.4 as T
import QtQuick.Controls 2.4
import QtQuick.Controls.impl 2.4
import QtQuick.Controls.Fusion 2.4
import QtQuick.Controls.Fusion.impl 2.4
import Cellink.Templates 1.0 as C

C.ComboBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(contentItem.implicitHeight,
                                      indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width + spacing)

    delegate: MenuItem {
        width: parent.width
        text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
    }

    indicator: ColorImage {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2
        color: control.editable ? control.palette.text : control.palette.buttonText
        source: "qrc:/qt-project.org/imports/QtQuick/Controls.2/Fusion/images/arrow.png"
        width: 20
        fillMode: Image.Pad
    }

    contentItem: T.TextField {
        topPadding: 4
        leftPadding: 4 - control.padding
        rightPadding: 4 - control.padding
        bottomPadding: 4

        text: control.editable ? control.editText : control.displayText

        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator

        font: control.font
        color: control.editable ? control.palette.text : control.palette.buttonText
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Text.AlignVCenter

        background: PaddedRectangle {
            clip: true
            radius: 2
            padding: 1
            leftPadding: control.mirrored ? -2 : padding
            rightPadding: !control.mirrored ? -2 : padding
            color: control.palette.base
            visible: control.editable && !control.flat

            Rectangle {
                x: parent.width - width
                y: 1
                width: 1
                height: parent.height - 2
                color: Fusion.buttonOutline(control.palette, control.activeFocus, control.enabled)
            }

            Rectangle {
                x: 1
                y: 1
                width: parent.width - 3
                height: 1
                color: Fusion.topShadow
            }
        }

        Rectangle {
            x: 1 - control.leftPadding
            y: 1
            width: control.width - 2
            height: control.height - 2
            color: "transparent"
            border.color: Color.transparent(Fusion.highlightedOutline(control.palette), 40 / 255)
            visible: control.activeFocus
            radius: 1.7
        }
    }

    background: ButtonPanel {
        implicitWidth: 120
        implicitHeight: 24

        control: control
        visible: !control.flat || control.down
        // ### TODO: fix control.contentItem.activeFocus
        highlighted: control.visualFocus || control.contentItem.activeFocus
    }

    popup: T.Popup {
        width: control.width
        height: Math.min(contentItem.implicitHeight + 2, control.Window.height - topMargin - bottomMargin)
        topMargin: 6
        bottomMargin: 6
        palette: control.palette
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightRangeMode: ListView.ApplyRange
            highlightMoveDuration: 0

            T.ScrollBar.vertical: ScrollBar { }
        }

        background: Rectangle {
            color: popup.palette.window
            border.color: Fusion.outline(control.palette)

            Rectangle {
                z: -1
                x: 1; y: 1
                width: parent.width
                height: parent.height
                color: control.palette.shadow
                opacity: 0.2
            }
        }
    }
}
