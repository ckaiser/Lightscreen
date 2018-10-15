/*
 * Copyright (C) 2017  Christian Kaiser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <QApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QStyle>
#include <QTimer>

#include <widgets/hotkeywidget.h>

HotkeyWidget::HotkeyWidget(QWidget *parent) :
    QPushButton(parent), mHotkey(QKeySequence()), mShowingError(false), mKeyboardFocus(false)
{
    mDefaultStyleSheet = "text-align: left; padding: 3px 6px;";
    setStyleSheet(mDefaultStyleSheet);

    setText(tr("Click to select hotkey..."));
    setObjectName("HotkeyWidget");

    if (qApp->style()->objectName() == "oxygen") {
        setMinimumWidth(130);
    } else {
        setMinimumWidth(110);
    }
}

void HotkeyWidget::setHotkey(const QString &hotkeyString)
{
    mHotkey = QKeySequence().fromString(hotkeyString, QKeySequence::NativeText);
    setHotkeyText();
}

QString HotkeyWidget::hotkey() const
{
    return mHotkey.toString(QKeySequence::PortableText);
}

void HotkeyWidget::showError()
{
    if (mShowingError) {
        return;
    }

    mShowingError = true;

    setStyleSheet(mDefaultStyleSheet + "color: #d90000;");
    QTimer::singleShot(1000, this, &HotkeyWidget::hideError);
}

void HotkeyWidget::setHotkeyText()
{
    QString hotkeyText = mHotkey.toString(QKeySequence::NativeText);

    setText(hotkeyText);
    parentWidget()->setFocus();
}

bool HotkeyWidget::event(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        setHotkeyText();
    } else if (event->type() == QEvent::KeyPress) {
        keyPressEvent(static_cast<QKeyEvent *>(event));
        return true;
    } else if (event->type() == QEvent::FocusIn) {
        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);

        if (focusEvent->reason() != Qt::TabFocusReason) {
            setText(tr("Type your hotkey"));
            mKeyboardFocus = false;
            grabKeyboard();
        } else {
            mKeyboardFocus = true;
        }
    } else if (event->type() == QEvent::FocusOut) {
        if (text() == tr("Invalid hotkey")) {
            emit invalidHotkeyError();
            showError();
        }

        releaseKeyboard();
        setHotkeyText(); // Reset the text
    } else if ((event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride || event->type() == QEvent::Shortcut) && hasFocus()) {
        event->accept();
        return true;
    }

    return QPushButton::event(event);
}

void HotkeyWidget::keyPressEvent(QKeyEvent *event)
{
    if (mKeyboardFocus) {
        return;
    }

    if (isModifier(event->key())) {
        return;
    }

    if (!isValid(event->key())) {
        setText(tr("Invalid hotkey"));
        parentWidget()->setFocus();
        return;
    }

    mHotkey = QKeySequence(event->key() + (event->modifiers() & ~Qt::KeypadModifier));

    setHotkeyText();
}

void HotkeyWidget::hideError()
{
    setStyleSheet(mDefaultStyleSheet);
    mShowingError = false;
}

bool HotkeyWidget::isValid(int key) const
{
    switch (key) {
    case 0:
    case Qt::Key_Escape:
    case Qt::Key_unknown:
        return false;
    }

    return !isModifier(key);
}

bool HotkeyWidget::isModifier(int key) const
{
    switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_Menu:
        return true;
    }

    return false;
}
