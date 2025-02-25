// Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "overlay-warning-plugin.h"

#include <QIcon>
#include <QStorageInfo>
#include <QDBusConnectionInterface>
#include <QTimer>

#include <ddialog.h>

#define PLUGIN_STATE_KEY    "enable"
#define OverlayFileSystemType    "overlay"
#define AuthAgentDbusService "org.deepin.dde.Polkit1.AuthAgent"

DWIDGET_USE_NAMESPACE

int WaitingAuthAgentTimes = 0;

OverlayWarningPlugin::OverlayWarningPlugin(QObject *parent)
    : QObject(parent)
    , m_pluginLoaded(false)
    , m_warningWidget(nullptr)
    , m_showDisableOverlayDialogTimer(new QTimer(this))
{
    m_showDisableOverlayDialogTimer->setInterval(6000);

    connect(m_showDisableOverlayDialogTimer, &QTimer::timeout, this, &OverlayWarningPlugin::showCloseOverlayDialogPre);
}

const QString OverlayWarningPlugin::pluginName() const
{
    return "overlay-warning";
}

const QString OverlayWarningPlugin::pluginDisplayName() const
{
    return QString();
}

QWidget *OverlayWarningPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return m_warningWidget.data();
}

QWidget *OverlayWarningPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return nullptr;
}

void OverlayWarningPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    if (!pluginIsDisable()) {
        loadPlugin();
    }
}

void OverlayWarningPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, PLUGIN_STATE_KEY, !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool());

    if (pluginIsDisable())
    {
        m_proxyInter->itemRemoved(this, pluginName());
    } else {
        if (!m_pluginLoaded) {
            loadPlugin();
            return;
        }
        m_proxyInter->itemAdded(this, pluginName());
    }
}

bool OverlayWarningPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool();
}

const QString OverlayWarningPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    m_showDisableOverlayDialogTimer->start(300);

    return QString();
}

void OverlayWarningPlugin::displayModeChanged(const Dock::DisplayMode displayMode)
{
    Q_UNUSED(displayMode);

    if (!pluginIsDisable()) {
        m_warningWidget->update();
    }
}

int OverlayWarningPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, 3).toInt();
}

void OverlayWarningPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

PluginFlags OverlayWarningPlugin::flags() const
{
    return PluginFlag::Type_NoneFlag;
}

void OverlayWarningPlugin::loadPlugin()
{
    if (m_pluginLoaded) {
        qDebug() << "overlay-warning plugin has been loaded! return";
        return;
    }

    m_pluginLoaded = true;

    m_warningWidget.reset(new OverlayWarningWidget);

    if (!isOverlayRoot()) {
        return;
    }

    m_proxyInter->itemAdded(this, pluginName());
    displayModeChanged(displayMode());

    QTimer::singleShot(0, m_showDisableOverlayDialogTimer, static_cast<void (QTimer::*) ()>(&QTimer::start));
}

bool OverlayWarningPlugin::isOverlayRoot()
{
    // ignore live/recovery mode
    QFile cmdline("/proc/cmdline");
    cmdline.open(QFile::ReadOnly);
    QString content(cmdline.readAll());
    cmdline.close();
    if (content.contains("boot=live")) {
        return false;
    }

    return QString(QStorageInfo::root().fileSystemType()) == OverlayFileSystemType;
}

void OverlayWarningPlugin::showCloseOverlayDialogPre()
{
    Q_ASSERT(sender() == m_showDisableOverlayDialogTimer);

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(AuthAgentDbusService)) {
        m_showDisableOverlayDialogTimer->stop();
        WaitingAuthAgentTimes = 0;
        showCloseOverlayDialog();
        return;
    }
    WaitingAuthAgentTimes++;
    qDebug() << "Waiting for AuthAgent service" << WaitingAuthAgentTimes << "times";
    if (WaitingAuthAgentTimes > 10) {
        qDebug() << "AuthAgent service timeout...";
        m_showDisableOverlayDialogTimer->stop();
        return;
    }
}

// Do not call this method directly
// It should be called by delay timer
// In order to check the Polkit dbus service is running
void OverlayWarningPlugin::showCloseOverlayDialog()
{
    qDebug() << "start disable overlayroot process";
    const int result = QProcess::execute("/usr/bin/pkexec /usr/sbin/overlayroot-disable", QStringList());
    if (result == 0) {
        QProcess::startDetached("reboot", QStringList());
    } else {
        qDebug() << "disable overlayroot failed, the return code is" << result;
    }
}
