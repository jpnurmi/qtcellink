/****************************************************************************
**
** Copyright (C) 2019 CELLINK AB <info@cellink.com>
** Copyright (C) 2017 The Qt Company Ltd.
**
** This file is part of QtCellink (based on the Qt Quick Templates 2 module of Qt).
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

#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

class QValidator;
class QQmlInstanceModel;
class ComboBoxPrivate;

class ComboBox : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QQmlInstanceModel *delegateModel READ delegateModel NOTIFY delegateModelChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL) // ### Qt 6: should not be writable
    Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged FINAL)
    Q_PROPERTY(QString displayText READ displayText WRITE setDisplayText RESET resetDisplayText NOTIFY displayTextChanged FINAL)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(QQuickItem *indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    Q_PROPERTY(QObject *popup READ popup WRITE setPopup NOTIFY popupChanged FINAL)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat NOTIFY flatChanged FINAL)
    Q_PROPERTY(bool down READ isDown WRITE setDown RESET resetDown NOTIFY downChanged FINAL)
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged FINAL)
    Q_PROPERTY(QString editText READ editText WRITE setEditText RESET resetEditText NOTIFY editTextChanged FINAL)
    Q_PROPERTY(QValidator *validator READ validator WRITE setValidator NOTIFY validatorChanged FINAL)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints NOTIFY inputMethodHintsChanged FINAL)
    Q_PROPERTY(bool inputMethodComposing READ isInputMethodComposing NOTIFY inputMethodComposingChanged FINAL)
    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput NOTIFY acceptableInputChanged FINAL)

public:
    explicit ComboBox(QQuickItem *parent = nullptr);
    ~ComboBox();

    int count() const;

    QVariant model() const;
    void setModel(const QVariant &model);
    QQmlInstanceModel *delegateModel() const;

    bool isPressed() const;
    void setPressed(bool pressed);

    int highlightedIndex() const;

    int currentIndex() const;
    void setCurrentIndex(int index);

    QString currentText() const;

    QString displayText() const;
    void setDisplayText(const QString &text);
    void resetDisplayText();

    QString textRole() const;
    void setTextRole(const QString &role);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QQuickItem *indicator() const;
    void setIndicator(QQuickItem *indicator);

    QObject *popup() const;
    void setPopup(QObject *popup);

    Q_INVOKABLE QString textAt(int index) const;
    Q_INVOKABLE int find(const QString &text, Qt::MatchFlags flags = Qt::MatchExactly) const;

    bool isFlat() const;
    void setFlat(bool flat);

    bool isDown() const;
    void setDown(bool down);
    void resetDown();

    bool isEditable() const;
    void setEditable(bool editable);

    QString editText() const;
    void setEditText(const QString &text);
    void resetEditText();

    QValidator *validator() const;
    void setValidator(QValidator *validator);

    Qt::InputMethodHints inputMethodHints() const;
    void setInputMethodHints(Qt::InputMethodHints hints);

    bool isInputMethodComposing() const;
    bool hasAcceptableInput() const;

public Q_SLOTS:
    void incrementCurrentIndex();
    void decrementCurrentIndex();
    void selectAll();

Q_SIGNALS:
    void activated(int index);
    void highlighted(int index);
    void countChanged();
    void modelChanged();
    void delegateModelChanged();
    void pressedChanged();
    void highlightedIndexChanged();
    void currentIndexChanged();
    void currentTextChanged();
    void displayTextChanged();
    void textRoleChanged();
    void delegateChanged();
    void indicatorChanged();
    void popupChanged();
    void flatChanged();
    void accepted();
    void downChanged();
    void editableChanged();
    void editTextChanged();
    void validatorChanged();
    void inputMethodHintsChanged();
    void inputMethodComposingChanged();
    void acceptableInputChanged();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
#if QT_CONFIG(im)
    void inputMethodEvent(QInputMethodEvent *event) override;
#endif
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif

    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override;

    QFont defaultFont() const override;
    QPalette defaultPalette() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(ComboBox)
    Q_DECLARE_PRIVATE(ComboBox)
};

QML_DECLARE_TYPE(ComboBox)

#endif // COMBOBOX_H
