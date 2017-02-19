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
#ifndef HOTKEYWIDGET_H
#define HOTKEYWIDGET_H

#include <QPushButton>

class HotkeyWidget : public QPushButton
{
    Q_OBJECT

public:
    HotkeyWidget(QWidget *parent = 0);

    void setHotkey(const QString &hotkey);
    QString hotkey() const;

signals:
    void invalidHotkeyError();

private:
    void setHotkeyText();
    void showError();

private slots:
    void hideError();

protected:
    // Event overrides:
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    QKeySequence mHotkey;
    bool mShowingError;
    bool mKeyboardFocus;
    QString mDefaultStyleSheet;

    bool isValid(int key) const;
    bool isModifier(int key) const;
};

#endif // HOTKEYWIDGET_H
