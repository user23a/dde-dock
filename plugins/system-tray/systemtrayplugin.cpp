#include "systemtrayplugin.h"
#include "fashiontrayitem.h"

#include <QWindow>
#include <QWidget>

#define FASHION_MODE_ITEM   "fashion-mode-item"

SystemTrayPlugin::SystemTrayPlugin(QObject *parent)
    : QObject(parent),
      m_trayInter(new DBusTrayManager(this)),
      m_tipsWidget(new TipsWidget)
{
    m_fashionItem = new FashionTrayItem;
}

const QString SystemTrayPlugin::pluginName() const
{
    return "system-tray";
}

void SystemTrayPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    connect(m_trayInter, &DBusTrayManager::Added, this, &SystemTrayPlugin::trayAdded);
    connect(m_trayInter, &DBusTrayManager::Removed, this, &SystemTrayPlugin::trayRemoved);
    connect(m_trayInter, &DBusTrayManager::Changed, this, &SystemTrayPlugin::trayChanged);

    m_trayInter->RetryManager();

    switchToMode(displayMode());
}

void SystemTrayPlugin::displayModeChanged(const Dock::DisplayMode mode)
{
    switchToMode(mode);
}

QWidget *SystemTrayPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == FASHION_MODE_ITEM)
        return m_fashionItem;

    const quint32 trayWinId = itemKey.toUInt();

    return m_trayList[trayWinId];
}

QWidget *SystemTrayPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_ASSERT(itemKey == FASHION_MODE_ITEM);
    Q_ASSERT(m_trayList.size());

    updateTipsContent();

    if (m_trayList.size() > 1)
        return m_tipsWidget;
    else
        return nullptr;
}

void SystemTrayPlugin::updateTipsContent()
{
    auto trayList = m_trayList.values();
//    trayList.removeOne(m_fashionItem->activeTray());

    m_tipsWidget->clear();
    m_tipsWidget->addWidgets(trayList);
}

void SystemTrayPlugin::trayAdded(const quint32 winId)
{
    if (m_trayList.contains(winId))
        return;

    TrayWidget *trayWidget = new TrayWidget(winId);

    m_trayList[winId] = trayWidget;

    m_fashionItem->setMouseEnable(m_trayList.size() == 1);

    if (displayMode() == Dock::Efficient)
        m_proxyInter->itemAdded(this, QString::number(winId));
}

void SystemTrayPlugin::trayRemoved(const quint32 winId)
{
    if (!m_trayList.contains(winId))
        return;

    TrayWidget *widget = m_trayList[winId];
    m_proxyInter->itemRemoved(this, QString::number(winId));
    m_trayList.remove(winId);
    widget->deleteLater();

    m_fashionItem->setMouseEnable(m_trayList.size() == 1);

    if (m_fashionItem->activeTray() != widget)
        return;
    // reset active tray
    if (m_trayList.values().isEmpty())
        m_fashionItem->setActiveTray(nullptr);
    else
        m_fashionItem->setActiveTray(m_trayList.values().last());

    if (m_tipsWidget->isVisible())
        updateTipsContent();
}

void SystemTrayPlugin::trayChanged(const quint32 winId)
{
    if (!m_trayList.contains(winId))
        return;

    m_fashionItem->setActiveTray(m_trayList[winId]);

    if (m_tipsWidget->isVisible())
        updateTipsContent();
}

void SystemTrayPlugin::switchToMode(const Dock::DisplayMode mode)
{
    if (mode == Dock::Fashion)
    {
        for (auto winId : m_trayList.keys())
            m_proxyInter->itemRemoved(this, QString::number(winId));
        m_proxyInter->itemAdded(this, FASHION_MODE_ITEM);
    }
    else
    {
        m_proxyInter->itemRemoved(this, FASHION_MODE_ITEM);
        for (auto winId : m_trayList.keys())
            m_proxyInter->itemAdded(this, QString::number(winId));
    }
}