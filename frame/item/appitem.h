// Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPITEM_H
#define APPITEM_H

#include "dockitem.h"
#include "previewcontainer.h"
#include "appdrag.h"
#include "../widgets/tipswidget.h"
#include "dbusutil.h"

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsItemAnimation>
#include <DGuiApplicationHelper>

class QGSettings;
class ScreenSpliter;

class AppItem : public DockItem
{
    Q_OBJECT

public:
    explicit AppItem(DockInter *dockInter, const QGSettings *appSettings, const QGSettings *activeAppSettings, const QGSettings *dockedAppSettings, const QDBusObjectPath &entry, QWidget *parent = nullptr);
    ~AppItem() override;

    void checkEntry() override;
    const QString appId() const;
    QString name() const;
    bool isValid() const;
    void updateWindowIconGeometries();
    void undock();
    QWidget *appDragWidget();
    void setDockInfo(Dock::Position dockPosition, const QRect &dockGeometry);
    void setDraging(bool drag) override;

    void startSplit(const QRect &rect);
    bool supportSplitWindow();
    bool splitWindowOnScreen(ScreenSpliter::SplitDirection direction);
    int mode() const;
    DockEntryInter *itemEntryInter() const;
    inline ItemType itemType() const override { return App; }
    QPixmap appIcon(){ return m_appIcon; }
    virtual QString accessibleName() override;
    void requestDock();
    bool isDocked() const;
    qint64 appOpenMSecs() const;
    void updateMSecs();
    const WindowInfoMap &windowsMap() const;

signals:
    void requestActivateWindow(const WId wid) const;
    void requestPreviewWindow(const WId wid) const;
    void requestCancelPreview() const;
    void dragReady(QWidget *dragWidget);

    void requestUpdateEntryGeometries() const;
    void windowCountChanged() const;
    void modeChanged(int) const;

private:
    void moveEvent(QMoveEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void showEvent(QShowEvent *e) override;

    void showHoverTips() override;
    void invokedMenuItem(const QString &itemId, const bool checked) override;
    const QString contextMenu() const override;
    QWidget *popupTips() override;
    void startDrag();
    bool hasAttention() const;

    QPoint appIconPosition() const;

private slots:
    void updateWindowInfos(const WindowInfoMap &info);
    void refreshIcon() override;
    void activeChanged();
    void showPreview();
    void playSwingEffect();
    void stopSwingEffect();
    void checkAttentionEffect();
    void onGSettingsChanged(const QString& key);
    bool checkGSettingsControl() const;
    void onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType);

    void onRefreshIcon();
    void onResetPreview();

private:
    const QGSettings *m_appSettings;
    const QGSettings *m_activeAppSettings;
    const QGSettings *m_dockedAppSettings;

    PreviewContainer *m_appPreviewTips;
    DockEntryInter *m_itemEntryInter;

    QGraphicsView *m_swingEffectView;
    QGraphicsItemAnimation *m_itemAnimation;

    DWindowManagerHelper *m_wmHelper;

    QPointer<AppDrag> m_drag;

    bool m_active;
    int m_retryTimes;
    bool m_iconValid;
    quint64 m_lastclickTimes;

    WindowInfoMap m_windowInfos;
    QString m_id;
    QPixmap m_appIcon;
    QPixmap m_horizontalIndicator;
    QPixmap m_verticalIndicator;
    QPixmap m_activeHorizontalIndicator;
    QPixmap m_activeVerticalIndicator;

    QTimer *m_updateIconGeometryTimer;
    QTimer *m_retryObtainIconTimer;
    QTimer *m_refershIconTimer;         // 当APP为日历时定时（1S）检测是否刷新ICON

    QDate m_curDate;                    // 保存当前icon的日期来判断是否需要更新日历APP的ICON

    DGuiApplicationHelper::ColorType m_themeType;
    qint64 m_createMSecs;

    static QPoint MousePressPos;

    ScreenSpliter *m_screenSpliter;
    DockInter *m_dockInter;
};

#endif // APPITEM_H
