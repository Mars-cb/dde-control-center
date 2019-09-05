/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGSGROUP_H
#define SETTINGSGROUP_H

#include <QFrame>
#include <QTimer>

#include "widgets/translucentframe.h"

class QVBoxLayout;

namespace dcc {
namespace widgets {

class SettingsItem;
class SettingsHeaderItem;

class SettingsGroup : public TranslucentFrame
{
    Q_OBJECT

public:
    enum BackgroupStyle {
        ItemBackground = 0,
        GroupBackgroud
    };

    explicit SettingsGroup(QFrame *parent = nullptr, BackgroupStyle bgStyle = ItemBackground);
    explicit SettingsGroup(const QString &title, QFrame *parent = nullptr);
    ~SettingsGroup();

    SettingsHeaderItem *headerItem() const { return m_headerItem; }
    void setHeaderVisible(const bool visible);

    SettingsItem *getItem(int index);
    void insertItem(const int index, SettingsItem *item);
    void appendItem(SettingsItem *item);
    void removeItem(SettingsItem *item);
    void moveItem(SettingsItem *item, const int index);
    void setSpacing(const int spaceing);

    int itemCount() const;
    void clear();

private:
    BackgroupStyle m_bgStyle{ItemBackground};
    QVBoxLayout *m_layout;
    SettingsHeaderItem *m_headerItem;
};

}
}

#endif // SETTINGSGROUP_H
