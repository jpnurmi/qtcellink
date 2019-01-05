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

#include "combobox.h"

#include <QtCore/qregexp.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtGui/qinputmethod.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/private/qlazilyallocated_p.h>
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquicktextinput_p.h>
#include <QtQuick/private/qquickitemview_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>

namespace {
    enum Activation { NoActivate, Activate };
    enum Highlighting { NoHighlight, Highlight };
}

class ComboBoxDelegateModel : public QQmlDelegateModel
{
public:
    explicit ComboBoxDelegateModel(ComboBox *combo);
    QString stringValue(int index, const QString &role) override;

private:
    ComboBox *combo = nullptr;
};

ComboBoxDelegateModel::ComboBoxDelegateModel(ComboBox *combo)
    : QQmlDelegateModel(qmlContext(combo), combo),
      combo(combo)
{
}

QString ComboBoxDelegateModel::stringValue(int index, const QString &role)
{
    QVariant model = combo->model();
    if (model.userType() == QMetaType::QVariantList) {
        QVariant object = model.toList().value(index);
        if (object.userType() == QMetaType::QVariantMap) {
            const QVariantMap data = object.toMap();
            if (data.count() == 1 && role == QLatin1String("modelData"))
                return data.first().toString();
            return data.value(role).toString();
        } else if (object.userType() == QMetaType::QObjectStar) {
            const QObject *data = object.value<QObject *>();
            if (data && role != QLatin1String("modelData"))
                return data->property(role.toUtf8()).toString();
        }
    }
    return QQmlDelegateModel::stringValue(index, role);
}

class ComboBoxPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(ComboBox)

public:
    bool isPopupVisible() const;
    void showPopup();
    void hidePopup(bool accept);
    void togglePopup(bool accept);
    void popupVisibleChanged();

    void itemClicked();
    void itemHovered();

    void createdItem(int index, QObject *object);
    void modelUpdated();
    void countChanged();

    void updateEditText();
    void updateCurrentText();

    void acceptInput();
    QString tryComplete(const QString &inputText);

    void incrementCurrentIndex();
    void decrementCurrentIndex();
    void setCurrentIndex(int index, Activation activate);
    void updateHighlightedIndex();
    void setHighlightedIndex(int index, Highlighting highlight);

    void keySearch(const QString &text);
    int match(int start, const QString &text, Qt::MatchFlags flags) const;

    void createDelegateModel();

    void handlePress(const QPointF &point) override;
    void handleMove(const QPointF &point) override;
    void handleRelease(const QPointF &point) override;
    void handleUngrab() override;

    void cancelIndicator();
    void executeIndicator(bool complete = false);

    void cancelPopup();
    void executePopup(bool complete = false);

    bool flat = false;
    bool down = false;
    bool hasDown = false;
    bool pressed = false;
    bool ownModel = false;
    bool keyNavigating = false;
    bool hasDisplayText = false;
    bool hasCurrentIndex = false;
    int highlightedIndex = -1;
    int currentIndex = -1;
    QVariant model;
    QString textRole;
    QString currentText;
    QString displayText;
    QQuickItem *pressedItem = nullptr;
    QQmlInstanceModel *delegateModel = nullptr;
    QQmlComponent *delegate = nullptr;
    QQuickItem *indicator = nullptr;
    QQuickPopup *popup = nullptr;

    struct ExtraData {
        bool editable = false;
        bool accepting = false;
        bool allowComplete = false;
        Qt::InputMethodHints inputMethodHints = Qt::ImhNone;
        QString editText;
        QValidator *validator = nullptr;
    };
    QLazilyAllocated<ExtraData> extra;
};

bool ComboBoxPrivate::isPopupVisible() const
{
    return popup && popup->isVisible();
}

void ComboBoxPrivate::showPopup()
{
    if (popup && !popup->isVisible())
        popup->open();
}

void ComboBoxPrivate::hidePopup(bool accept)
{
    Q_Q(ComboBox);
    if (accept) {
        q->setCurrentIndex(highlightedIndex);
        emit q->activated(currentIndex);
    }
    if (popup && popup->isVisible())
        popup->close();
}

void ComboBoxPrivate::togglePopup(bool accept)
{
    if (!popup || !popup->isVisible())
        showPopup();
    else
        hidePopup(accept);
}

void ComboBoxPrivate::popupVisibleChanged()
{
    Q_Q(ComboBox);
    if (isPopupVisible())
        QGuiApplication::inputMethod()->reset();

    QQuickItemView *itemView = popup->findChild<QQuickItemView *>();
    if (itemView)
        itemView->setHighlightRangeMode(QQuickItemView::NoHighlightRange);

    updateHighlightedIndex();

    if (itemView)
        itemView->positionViewAtIndex(highlightedIndex, QQuickItemView::Beginning);

    if (!hasDown) {
        q->setDown(pressed || isPopupVisible());
        hasDown = false;
    }
}

void ComboBoxPrivate::itemClicked()
{
    Q_Q(ComboBox);
    int index = delegateModel->indexOf(q->sender(), nullptr);
    if (index != -1) {
        setHighlightedIndex(index, Highlight);
        hidePopup(true);
    }
}

void ComboBoxPrivate::itemHovered()
{
    Q_Q(ComboBox);
    if (keyNavigating)
        return;

    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(q->sender());
    if (!button || !button->isHovered() || QQuickAbstractButtonPrivate::get(button)->touchId != -1)
        return;

    int index = delegateModel->indexOf(button, nullptr);
    if (index != -1) {
        setHighlightedIndex(index, Highlight);

        if (QQuickItemView *itemView = popup->findChild<QQuickItemView *>())
            itemView->positionViewAtIndex(index, QQuickItemView::Contain);
    }
}

void ComboBoxPrivate::createdItem(int index, QObject *object)
{
    Q_Q(ComboBox);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (item && !item->parentItem()) {
        if (popup)
            item->setParentItem(popup->contentItem());
        else
            item->setParentItem(q);
        QQuickItemPrivate::get(item)->setCulled(true);
    }

    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(object);
    if (button) {
        button->setFocusPolicy(Qt::NoFocus);
        connect(button, &QQuickAbstractButton::clicked, this, &ComboBoxPrivate::itemClicked);
        connect(button, &QQuickAbstractButton::hoveredChanged, this, &ComboBoxPrivate::itemHovered);
    }

    if (index == currentIndex && !q->isEditable())
        updateCurrentText();
}

void ComboBoxPrivate::modelUpdated()
{
    if (!extra.isAllocated() || !extra->accepting)
        updateCurrentText();
}

void ComboBoxPrivate::countChanged()
{
    Q_Q(ComboBox);
    if (q->count() == 0)
        q->setCurrentIndex(-1);
    emit q->countChanged();
}

void ComboBoxPrivate::updateEditText()
{
    Q_Q(ComboBox);
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(contentItem);
    if (!input)
        return;

    const QString text = input->text();

    if (extra.isAllocated() && extra->allowComplete && !text.isEmpty()) {
        const QString completed = tryComplete(text);
        if (completed.length() > text.length()) {
            input->setText(completed);
            input->select(completed.length(), text.length());
            return;
        }
    }
    q->setEditText(text);
}

void ComboBoxPrivate::updateCurrentText()
{
    Q_Q(ComboBox);
    QString text = q->textAt(currentIndex);
    if (currentText != text) {
        currentText = text;
        if (!hasDisplayText)
           q->setAccessibleName(text);
        emit q->currentTextChanged();
    }
    if (!hasDisplayText && displayText != text) {
        displayText = text;
        emit q->displayTextChanged();
    }
    if (!extra.isAllocated() || !extra->accepting)
        q->setEditText(currentText);
}

void ComboBoxPrivate::acceptInput()
{
    Q_Q(ComboBox);
    int idx = q->find(extra.value().editText, Qt::MatchFixedString);
    if (idx > -1)
        q->setCurrentIndex(idx);

    extra.value().accepting = true;
    emit q->accepted();

    if (idx == -1)
        q->setCurrentIndex(q->find(extra.value().editText, Qt::MatchFixedString));
    extra.value().accepting = false;
}

QString ComboBoxPrivate::tryComplete(const QString &input)
{
    Q_Q(ComboBox);
    QString match;

    const int itemCount = q->count();
    for (int idx = 0; idx < itemCount; ++idx) {
        const QString text = q->textAt(idx);
        if (!text.startsWith(input, Qt::CaseInsensitive))
            continue;

        // either the first or the shortest match
        if (match.isEmpty() || text.length() < match.length())
            match = text;
    }

    if (match.isEmpty())
        return input;

    return input + match.mid(input.length());
}

void ComboBoxPrivate::setCurrentIndex(int index, Activation activate)
{
    Q_Q(ComboBox);
    if (currentIndex == index)
        return;

    currentIndex = index;
    emit q->currentIndexChanged();

    if (componentComplete)
        updateCurrentText();

    if (activate)
        emit q->activated(index);
}

void ComboBoxPrivate::incrementCurrentIndex()
{
    Q_Q(ComboBox);
    if (extra.isAllocated())
        extra->allowComplete = false;
    if (isPopupVisible()) {
        if (highlightedIndex < q->count() - 1)
            setHighlightedIndex(highlightedIndex + 1, Highlight);
    } else {
        if (currentIndex < q->count() - 1)
            setCurrentIndex(currentIndex + 1, Activate);
    }
    if (extra.isAllocated())
        extra->allowComplete = true;
}

void ComboBoxPrivate::decrementCurrentIndex()
{
    if (extra.isAllocated())
        extra->allowComplete = false;
    if (isPopupVisible()) {
        if (highlightedIndex > 0)
            setHighlightedIndex(highlightedIndex - 1, Highlight);
    } else {
        if (currentIndex > 0)
            setCurrentIndex(currentIndex - 1, Activate);
    }
    if (extra.isAllocated())
        extra->allowComplete = true;
}

void ComboBoxPrivate::updateHighlightedIndex()
{
    setHighlightedIndex(popup->isVisible() ? currentIndex : -1, NoHighlight);
}

void ComboBoxPrivate::setHighlightedIndex(int index, Highlighting highlight)
{
    Q_Q(ComboBox);
    if (highlightedIndex == index)
        return;

    highlightedIndex = index;
    emit q->highlightedIndexChanged();

    if (highlight)
        emit q->highlighted(index);
}

void ComboBoxPrivate::keySearch(const QString &text)
{
    const int startIndex = isPopupVisible() ? highlightedIndex : currentIndex;
    const int index = match(startIndex + 1, text, Qt::MatchStartsWith | Qt::MatchWrap);
    if (index != -1) {
        if (isPopupVisible())
            setHighlightedIndex(index, Highlight);
        else
            setCurrentIndex(index, Activate);
    }
}

int ComboBoxPrivate::match(int start, const QString &text, Qt::MatchFlags flags) const
{
    Q_Q(const ComboBox);
    uint matchType = flags & 0x0F;
    bool wrap = flags & Qt::MatchWrap;
    Qt::CaseSensitivity cs = flags & Qt::MatchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int from = start;
    int to = q->count();

    // iterates twice if wrapping
    for (int i = 0; (wrap && i < 2) || (!wrap && i < 1); ++i) {
        for (int idx = from; idx < to; ++idx) {
            QString t = q->textAt(idx);
            switch (matchType) {
            case Qt::MatchExactly:
                if (t == text)
                    return idx;
                break;
            case Qt::MatchRegExp:
                if (QRegExp(text, cs).exactMatch(t))
                    return idx;
                break;
            case Qt::MatchWildcard:
                if (QRegExp(text, cs, QRegExp::Wildcard).exactMatch(t))
                    return idx;
                break;
            case Qt::MatchStartsWith:
                if (t.startsWith(text, cs))
                    return idx;
                break;
            case Qt::MatchEndsWith:
                if (t.endsWith(text, cs))
                    return idx;
                break;
            case Qt::MatchFixedString:
                if (t.compare(text, cs) == 0)
                    return idx;
                break;
            case Qt::MatchContains:
            default:
                if (t.contains(text, cs))
                    return idx;
                break;
            }
        }
        // prepare for the next iteration
        from = 0;
        to = start;
    }
    return -1;
}

void ComboBoxPrivate::createDelegateModel()
{
    Q_Q(ComboBox);
    bool ownedOldModel = ownModel;
    QQmlInstanceModel* oldModel = delegateModel;
    if (oldModel) {
        disconnect(delegateModel, &QQmlInstanceModel::countChanged, this, &ComboBoxPrivate::countChanged);
        disconnect(delegateModel, &QQmlInstanceModel::modelUpdated, this, &ComboBoxPrivate::modelUpdated);
        disconnect(delegateModel, &QQmlInstanceModel::createdItem, this, &ComboBoxPrivate::createdItem);
    }

    ownModel = false;
    delegateModel = model.value<QQmlInstanceModel *>();

    if (!delegateModel && model.isValid()) {
        QQmlDelegateModel *dataModel = new ComboBoxDelegateModel(q);
        dataModel->setModel(model);
        dataModel->setDelegate(delegate);
        if (q->isComponentComplete())
            dataModel->componentComplete();

        ownModel = true;
        delegateModel = dataModel;
    }

    if (delegateModel) {
        connect(delegateModel, &QQmlInstanceModel::countChanged, this, &ComboBoxPrivate::countChanged);
        connect(delegateModel, &QQmlInstanceModel::modelUpdated, this, &ComboBoxPrivate::modelUpdated);
        connect(delegateModel, &QQmlInstanceModel::createdItem, this, &ComboBoxPrivate::createdItem);
    }

    emit q->delegateModelChanged();

    if (ownedOldModel)
        delete oldModel;
}

void ComboBoxPrivate::handlePress(const QPointF &point)
{
    Q_Q(ComboBox);
    QQuickControlPrivate::handlePress(point);
    q->setPressed(true);
}

void ComboBoxPrivate::handleMove(const QPointF &point)
{
    Q_Q(ComboBox);
    QQuickControlPrivate::handleMove(point);
    q->setPressed(q->contains(point));
}

void ComboBoxPrivate::handleRelease(const QPointF &point)
{
    Q_Q(ComboBox);
    QQuickControlPrivate::handleRelease(point);
    if (pressed) {
        q->setPressed(false);
        togglePopup(false);
    }
}

void ComboBoxPrivate::handleUngrab()
{
    Q_Q(ComboBox);
    QQuickControlPrivate::handleUngrab();
    q->setPressed(false);
}

ComboBox::ComboBox(QQuickItem *parent)
    : QQuickControl(*(new ComboBoxPrivate), parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif
    setInputMethodHints(Qt::ImhNoPredictiveText);
}

ComboBox::~ComboBox()
{
    Q_D(ComboBox);
    if (d->popup) {
        // Disconnect visibleChanged() to avoid a spurious highlightedIndexChanged() signal
        // emission during the destruction of the (visible) popup. (QTBUG-57650)
        QObjectPrivate::disconnect(d->popup, &QQuickPopup::visibleChanged, d, &ComboBoxPrivate::popupVisibleChanged);
        delete d->popup;
        d->popup = nullptr;
    }
}

int ComboBox::count() const
{
    Q_D(const ComboBox);
    return d->delegateModel ? d->delegateModel->count() : 0;
}

QVariant ComboBox::model() const
{
    Q_D(const ComboBox);
    return d->model;
}

void ComboBox::setModel(const QVariant& m)
{
    Q_D(ComboBox);
    QVariant model = m;
    if (model.userType() == qMetaTypeId<QJSValue>())
        model = model.value<QJSValue>().toVariant();

    if (d->model == model)
        return;

    if (QAbstractItemModel* aim = qvariant_cast<QAbstractItemModel *>(d->model))
        QObjectPrivate::disconnect(aim, &QAbstractItemModel::dataChanged, d, &ComboBoxPrivate::updateCurrentText);
    if (QAbstractItemModel* aim = qvariant_cast<QAbstractItemModel *>(model))
        QObjectPrivate::connect(aim, &QAbstractItemModel::dataChanged, d, &ComboBoxPrivate::updateCurrentText);

    d->model = model;
    d->createDelegateModel();
    if (isComponentComplete()) {
        setCurrentIndex(count() > 0 ? 0 : -1);
        d->updateCurrentText();
    }
    emit modelChanged();
}

QQmlInstanceModel *ComboBox::delegateModel() const
{
    Q_D(const ComboBox);
    return d->delegateModel;
}

bool ComboBox::isPressed() const
{
    Q_D(const ComboBox);
    return d->pressed;
}

void ComboBox::setPressed(bool pressed)
{
    Q_D(ComboBox);
    if (d->pressed == pressed)
        return;

    d->pressed = pressed;
    emit pressedChanged();

    if (!d->hasDown) {
        setDown(d->pressed || d->isPopupVisible());
        d->hasDown = false;
    }
}

int ComboBox::highlightedIndex() const
{
    Q_D(const ComboBox);
    return d->highlightedIndex;
}

int ComboBox::currentIndex() const
{
    Q_D(const ComboBox);
    return d->currentIndex;
}

void ComboBox::setCurrentIndex(int index)
{
    Q_D(ComboBox);
    d->hasCurrentIndex = true;
    d->setCurrentIndex(index, NoActivate);
}

QString ComboBox::currentText() const
{
    Q_D(const ComboBox);
    return d->currentText;
}

QString ComboBox::displayText() const
{
    Q_D(const ComboBox);
    return d->displayText;
}

void ComboBox::setDisplayText(const QString &text)
{
    Q_D(ComboBox);
    d->hasDisplayText = true;
    if (d->displayText == text)
        return;

    d->displayText = text;
    setAccessibleName(text);
    emit displayTextChanged();
}

void ComboBox::resetDisplayText()
{
    Q_D(ComboBox);
    if (!d->hasDisplayText)
        return;

    d->hasDisplayText = false;
    d->updateCurrentText();
}

QString ComboBox::textRole() const
{
    Q_D(const ComboBox);
    return d->textRole;
}

void ComboBox::setTextRole(const QString &role)
{
    Q_D(ComboBox);
    if (d->textRole == role)
        return;

    d->textRole = role;
    if (isComponentComplete())
        d->updateCurrentText();
    emit textRoleChanged();
}

QQmlComponent *ComboBox::delegate() const
{
    Q_D(const ComboBox);
    return d->delegate;
}

void ComboBox::setDelegate(QQmlComponent* delegate)
{
    Q_D(ComboBox);
    if (d->delegate == delegate)
        return;

    delete d->delegate;
    d->delegate = delegate;
    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel*>(d->delegateModel);
    if (delegateModel)
        delegateModel->setDelegate(d->delegate);
    emit delegateChanged();
}

QQuickItem *ComboBox::indicator() const
{
    Q_D(const ComboBox);
    return d->indicator;
}

void ComboBox::setIndicator(QQuickItem *indicator)
{
    Q_D(ComboBox);
    if (d->indicator == indicator)
        return;

    delete d->indicator;
    d->indicator = indicator;
    if (indicator) {
        if (!indicator->parentItem())
            indicator->setParentItem(this);
    }
    emit indicatorChanged();
}

QQuickPopup *ComboBox::popup() const
{
    Q_D(const ComboBox);
    return d->popup;
}

void ComboBox::setPopup(QQuickPopup *popup)
{
    Q_D(ComboBox);
    if (d->popup == popup)
        return;

    if (d->popup) {
        QObjectPrivate::disconnect(d->popup, &QQuickPopup::visibleChanged, d, &ComboBoxPrivate::popupVisibleChanged);
        delete d->popup;
    }
    if (popup) {
        QQuickPopupPrivate::get(popup)->allowVerticalFlip = true;
        popup->setClosePolicy(QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutsideParent);
        QObjectPrivate::connect(popup, &QQuickPopup::visibleChanged, d, &ComboBoxPrivate::popupVisibleChanged);

        if (QQuickItemView *itemView = popup->findChild<QQuickItemView *>())
            itemView->setHighlightRangeMode(QQuickItemView::NoHighlightRange);
    }
    d->popup = popup;
    emit popupChanged();
}

bool ComboBox::isFlat() const
{
    Q_D(const ComboBox);
    return d->flat;
}

void ComboBox::setFlat(bool flat)
{
    Q_D(ComboBox);
    if (d->flat == flat)
        return;

    d->flat = flat;
    emit flatChanged();
}

bool ComboBox::isDown() const
{
    Q_D(const ComboBox);
    return d->down;
}

void ComboBox::setDown(bool down)
{
    Q_D(ComboBox);
    d->hasDown = true;

    if (d->down == down)
        return;

    d->down = down;
    emit downChanged();
}

void ComboBox::resetDown()
{
    Q_D(ComboBox);
    if (!d->hasDown)
        return;

    setDown(d->pressed || d->isPopupVisible());
    d->hasDown = false;
}

bool ComboBox::isEditable() const
{
    Q_D(const ComboBox);
    return d->extra.isAllocated() && d->extra->editable;
}

void ComboBox::setEditable(bool editable)
{
    Q_D(ComboBox);
    if (editable == isEditable())
        return;

    if (d->contentItem) {
        if (editable) {
            d->contentItem->installEventFilter(this);
            if (QQuickTextInput *input = qobject_cast<QQuickTextInput *>(d->contentItem)) {
                QObjectPrivate::connect(input, &QQuickTextInput::textChanged, d, &ComboBoxPrivate::updateEditText);
                QObjectPrivate::connect(input, &QQuickTextInput::accepted, d, &ComboBoxPrivate::acceptInput);
            }
#if QT_CONFIG(cursor)
            d->contentItem->setCursor(Qt::IBeamCursor);
#endif
        } else {
            d->contentItem->removeEventFilter(this);
            if (QQuickTextInput *input = qobject_cast<QQuickTextInput *>(d->contentItem)) {
                QObjectPrivate::disconnect(input, &QQuickTextInput::textChanged, d, &ComboBoxPrivate::updateEditText);
                QObjectPrivate::disconnect(input, &QQuickTextInput::accepted, d, &ComboBoxPrivate::acceptInput);
            }
#if QT_CONFIG(cursor)
            d->contentItem->unsetCursor();
#endif
        }
    }

    d->extra.value().editable = editable;
    setAccessibleProperty("editable", editable);
    emit editableChanged();
}

QString ComboBox::editText() const
{
    Q_D(const ComboBox);
    return d->extra.isAllocated() ? d->extra->editText : QString();
}

void ComboBox::setEditText(const QString &text)
{
    Q_D(ComboBox);
    if (text == editText())
        return;

    d->extra.value().editText = text;
    emit editTextChanged();
}

void ComboBox::resetEditText()
{
    setEditText(QString());
}

QValidator *ComboBox::validator() const
{
    Q_D(const ComboBox);
    return d->extra.isAllocated() ? d->extra->validator : nullptr;
}

void ComboBox::setValidator(QValidator *validator)
{
    Q_D(ComboBox);
    if (validator == ComboBox::validator())
        return;

    d->extra.value().validator = validator;
#if QT_CONFIG(validator)
    if (validator)
        validator->setLocale(d->locale);
#endif
    emit validatorChanged();
}

Qt::InputMethodHints ComboBox::inputMethodHints() const
{
    Q_D(const ComboBox);
    return d->extra.isAllocated() ? d->extra->inputMethodHints : Qt::ImhNoPredictiveText;
}

void ComboBox::setInputMethodHints(Qt::InputMethodHints hints)
{
    Q_D(ComboBox);
    if (hints == inputMethodHints())
        return;

    d->extra.value().inputMethodHints = hints;
    emit inputMethodHintsChanged();
}

bool ComboBox::isInputMethodComposing() const
{
    Q_D(const ComboBox);
    return d->contentItem && d->contentItem->property("inputMethodComposing").toBool();
}

bool ComboBox::hasAcceptableInput() const
{
    Q_D(const ComboBox);
    return d->contentItem && d->contentItem->property("acceptableInput").toBool();
}

QString ComboBox::textAt(int index) const
{
    Q_D(const ComboBox);
    if (!d->delegateModel || index < 0 || index >= d->delegateModel->count())
        return QString();

    QString text;
    QObject *object = d->delegateModel->object(index);
    if (object) {
        text = d->delegateModel->stringValue(index, d->textRole.isEmpty() ? QStringLiteral("modelData") : d->textRole);
        d->delegateModel->release(object);
    }
    return text;
}

int ComboBox::find(const QString &text, Qt::MatchFlags flags) const
{
    Q_D(const ComboBox);
    return d->match(0, text, flags);
}

void ComboBox::incrementCurrentIndex()
{
    Q_D(ComboBox);
    d->incrementCurrentIndex();
}

void ComboBox::decrementCurrentIndex()
{
    Q_D(ComboBox);
    d->decrementCurrentIndex();
}

void ComboBox::selectAll()
{
    Q_D(ComboBox);
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(d->contentItem);
    if (!input)
        return;
    input->selectAll();
}

bool ComboBox::eventFilter(QObject *object, QEvent *event)
{
    Q_D(ComboBox);
    switch (event->type()) {
    case QEvent::MouseButtonRelease:
        if (d->isPopupVisible())
            d->hidePopup(false);
        break;
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (d->filterKeyEvent(ke, false))
            return true;
        event->accept();
        if (d->extra.isAllocated())
            d->extra->allowComplete = ke->key() != Qt::Key_Backspace && ke->key() != Qt::Key_Delete;
        break;
    }
    case QEvent::FocusOut:
        d->hidePopup(false);
        setPressed(false);
        break;
#if QT_CONFIG(im)
    case QEvent::InputMethod:
        if (d->extra.isAllocated())
            d->extra->allowComplete = !static_cast<QInputMethodEvent*>(event)->commitString().isEmpty();
        break;
#endif
    default:
        break;
    }
    return QQuickControl::eventFilter(object, event);
}

void ComboBox::focusInEvent(QFocusEvent *event)
{
    Q_D(ComboBox);
    QQuickControl::focusInEvent(event);
    if (d->contentItem && isEditable())
        d->contentItem->forceActiveFocus(event->reason());
}

void ComboBox::focusOutEvent(QFocusEvent *event)
{
    Q_D(ComboBox);
    QQuickControl::focusOutEvent(event);
    d->hidePopup(false);
    setPressed(false);
}

#if QT_CONFIG(im)
void ComboBox::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(ComboBox);
    QQuickControl::inputMethodEvent(event);
    if (!isEditable() && !event->commitString().isEmpty())
        d->keySearch(event->commitString());
    else
        event->ignore();
}
#endif

void ComboBox::keyPressEvent(QKeyEvent *event)
{
    Q_D(ComboBox);
    QQuickControl::keyPressEvent(event);

    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Back:
        if (d->isPopupVisible())
            event->accept();
        break;
    case Qt::Key_Space:
        if (!event->isAutoRepeat())
            setPressed(true);
        event->accept();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (d->isPopupVisible())
            setPressed(true);
        event->accept();
        break;
    case Qt::Key_Up:
        d->keyNavigating = true;
        d->decrementCurrentIndex();
        event->accept();
        break;
    case Qt::Key_Down:
        d->keyNavigating = true;
        d->incrementCurrentIndex();
        event->accept();
        break;
    case Qt::Key_Home:
        d->keyNavigating = true;
        if (d->isPopupVisible())
            d->setHighlightedIndex(0, Highlight);
        else
            d->setCurrentIndex(0, Activate);
        event->accept();
        break;
    case Qt::Key_End:
        d->keyNavigating = true;
        if (d->isPopupVisible())
            d->setHighlightedIndex(count() - 1, Highlight);
        else
            d->setCurrentIndex(count() - 1, Activate);
        event->accept();
        break;
    default:
        if (!isEditable() && !event->text().isEmpty())
            d->keySearch(event->text());
        else
            event->ignore();
        break;
    }
}

void ComboBox::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(ComboBox);
    QQuickControl::keyReleaseEvent(event);
    d->keyNavigating = false;
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_Space:
        if (!isEditable())
            d->togglePopup(true);
        setPressed(false);
        event->accept();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (!isEditable() || d->isPopupVisible())
            d->hidePopup(d->isPopupVisible());
        setPressed(false);
        event->accept();
        break;
    case Qt::Key_Escape:
    case Qt::Key_Back:
        if (d->isPopupVisible()) {
            d->hidePopup(false);
            setPressed(false);
            event->accept();
        }
        break;
    default:
        break;
    }
}

#if QT_CONFIG(wheelevent)
void ComboBox::wheelEvent(QWheelEvent *event)
{
    Q_D(ComboBox);
    QQuickControl::wheelEvent(event);
    if (d->wheelEnabled && !d->isPopupVisible()) {
        if (event->angleDelta().y() > 0)
            d->decrementCurrentIndex();
        else
            d->incrementCurrentIndex();
    }
}
#endif

void ComboBox::componentComplete()
{
    Q_D(ComboBox);
    QQuickControl::componentComplete();

    if (d->delegateModel && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->delegateModel)->componentComplete();

    if (count() > 0) {
        if (!d->hasCurrentIndex && d->currentIndex == -1)
            setCurrentIndex(0);
        else
            d->updateCurrentText();
    }
}

void ComboBox::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(ComboBox);
    QQuickControl::itemChange(change, value);
    if (change == ItemVisibleHasChanged && !value.boolValue) {
        d->hidePopup(false);
        setPressed(false);
    }
}

void ComboBox::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(ComboBox);
    if (oldItem) {
        oldItem->removeEventFilter(this);
        if (QQuickTextInput *oldInput = qobject_cast<QQuickTextInput *>(oldItem)) {
            QObjectPrivate::disconnect(oldInput, &QQuickTextInput::accepted, d, &ComboBoxPrivate::acceptInput);
            QObjectPrivate::disconnect(oldInput, &QQuickTextInput::textChanged, d, &ComboBoxPrivate::updateEditText);
            disconnect(oldInput, &QQuickTextInput::inputMethodComposingChanged, this, &ComboBox::inputMethodComposingChanged);
            disconnect(oldInput, &QQuickTextInput::acceptableInputChanged, this, &ComboBox::acceptableInputChanged);
        }
    }
    if (newItem && isEditable()) {
        newItem->installEventFilter(this);
        if (QQuickTextInput *newInput = qobject_cast<QQuickTextInput *>(newItem)) {
            QObjectPrivate::connect(newInput, &QQuickTextInput::accepted, d, &ComboBoxPrivate::acceptInput);
            QObjectPrivate::connect(newInput, &QQuickTextInput::textChanged, d, &ComboBoxPrivate::updateEditText);
            connect(newInput, &QQuickTextInput::inputMethodComposingChanged, this, &ComboBox::inputMethodComposingChanged);
            connect(newInput, &QQuickTextInput::acceptableInputChanged, this, &ComboBox::acceptableInputChanged);
        }
#if QT_CONFIG(cursor)
        newItem->setCursor(Qt::IBeamCursor);
#endif
    }
}

void ComboBox::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    QQuickControl::localeChange(newLocale, oldLocale);
#if QT_CONFIG(validator)
    if (QValidator *v = validator())
        v->setLocale(newLocale);
#endif
}

QFont ComboBox::defaultFont() const
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 12, 0))
    return QQuickControlPrivate::themeFont(QPlatformTheme::ComboMenuItemFont);
#else
    return QQuickTheme::font(QQuickTheme::ComboBox);
#endif
}

QPalette ComboBox::defaultPalette() const
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 12, 0))
    return QQuickControlPrivate::themePalette(QPlatformTheme::ComboBoxPalette);
#else
    return QQuickTheme::palette(QQuickTheme::ComboBox);
#endif
}

#if QT_CONFIG(accessibility)
QAccessible::Role ComboBox::accessibleRole() const
{
    return QAccessible::ComboBox;
}

void ComboBox::accessibilityActiveChanged(bool active)
{
    Q_D(ComboBox);
    QQuickControl::accessibilityActiveChanged(active);

    if (active) {
        setAccessibleName(d->hasDisplayText ? d->displayText : d->currentText);
        setAccessibleProperty("editable", isEditable());
    }
}
#endif //
