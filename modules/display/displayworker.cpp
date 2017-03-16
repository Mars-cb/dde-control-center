#include "displayworker.h"
#include "displaymodel.h"
#include "monitorsettingdialog.h"

#include <QDebug>

using namespace dcc;
using namespace dcc::display;

const QString DisplayInterface("com.deepin.daemon.Display");

DisplayWorker::DisplayWorker(DisplayModel *model, QObject *parent)
    : QObject(parent),

      m_model(model),

      m_displayInter(DisplayInterface, "/com/deepin/daemon/Display", QDBusConnection::sessionBus(), this)
{
    // TODO:
    model->setPrimary(m_displayInter.primary());

    m_displayInter.setSync(false);

    connect(&m_displayInter, &DisplayInter::MonitorsChanged, this, &DisplayWorker::onMonitorListChanged);
    connect(&m_displayInter, &DisplayInter::BrightnessChanged, this, &DisplayWorker::onMonitorsBrightnessChanged);
    connect(&m_displayInter, &DisplayInter::ScreenHeightChanged, model, &DisplayModel::setScreenHeight);
    connect(&m_displayInter, &DisplayInter::ScreenWidthChanged, model, &DisplayModel::setScreenWidth);
    connect(&m_displayInter, &DisplayInter::DisplayModeChanged, model, &DisplayModel::setDisplayMode);
    connect(&m_displayInter, &DisplayInter::CurrentCustomIdChanged, model, &DisplayModel::setCurrentConfig);
    connect(&m_displayInter, &DisplayInter::CustomIdListChanged, model, &DisplayModel::setConfigList);
//    connect(&m_displayInter, &DisplayInter::HasCustomConfigChanged, model, &DisplayModel::setHasConfig);
    connect(&m_displayInter, static_cast<void (DisplayInter::*)(const QString &) const>(&DisplayInter::PrimaryChanged), model, &DisplayModel::setPrimary);

    onMonitorListChanged(m_displayInter.monitors());
    onMonitorsBrightnessChanged(m_displayInter.brightness());
    model->setScreenHeight(m_displayInter.screenHeight());
    model->setScreenWidth(m_displayInter.screenWidth());
    model->setConfigList(m_displayInter.customIdList());
    model->setCurrentConfig(m_displayInter.currentCustomId());
//    model->setHasConfig(m_displayInter.hasCustomConfig());
    model->setDisplayMode(m_displayInter.displayMode());
}

DisplayWorker::~DisplayWorker()
{
    qDeleteAll(m_monitors.keys());
    qDeleteAll(m_monitors.values());
}

void DisplayWorker::saveChanges()
{
    m_displayInter.Save();
}

void DisplayWorker::discardChanges()
{
    m_displayInter.ResetChanges().waitForFinished();
}

void DisplayWorker::mergeScreens()
{
    const auto mList = m_model->monitorList();
    Q_ASSERT(mList.size() == 2);

    const auto rotate = m_model->primaryMonitor()->rotate();
    const auto brightness = m_model->primaryMonitor()->brightness();
    const auto mode = m_model->monitorsSameModeList().first();
    for (auto *mon : mList)
    {
        auto *mInter = m_monitors[mon];
        Q_ASSERT(mInter);

        m_displayInter.SetBrightness(mInter->name(), brightness);
        mInter->SetPosition(0, 0).waitForFinished();
        mInter->SetRotation(rotate).waitForFinished();
        mInter->SetModeBySize(mode.width(), mode.height()).waitForFinished();
    }

    m_displayInter.ApplyChanges();
}

void DisplayWorker::splitScreens()
{
    const auto mList = m_model->monitorList();
    Q_ASSERT(mList.size() == 2);

    const auto *primary = m_model->primaryMonitor();

    int xOffset = primary->w();
    for (auto *mon : mList)
    {
        // pass primary
        if (mon == primary)
            continue;

        auto *mInter = m_monitors[mon];
        Q_ASSERT(mInter);

        mInter->SetPosition(xOffset, 0).waitForFinished();
        xOffset += mon->w();
    }

    m_displayInter.ApplyChanges();
}

void DisplayWorker::createConfig()
{
    int idx = 0;
    QString configName;

    do
    {
        configName = tr("My Settings %1").arg(++idx);

        if (!m_model->configList().contains(configName))
            break;
    } while (true);

    switchConfig(configName);
}

void DisplayWorker::switchConfig(const QString &config)
{
    switchMode(CUSTOM_MODE, config);
}

void DisplayWorker::deleteConfig(const QString &config)
{
    m_displayInter.DeleteCustomMode(config);
}

//void DisplayWorker::switchCustom(const bool deleteConfig)
//{
    // delete old config file
//    if (deleteConfig)
//        m_displayInter.DeleteCustomConfig().waitForFinished();

//    switchMode(CUSTOM_MODE);
//}

void DisplayWorker::switchMode(const int mode, const QString &name)
{
    qDebug() << Q_FUNC_INFO << mode << name;

    m_displayInter.SwitchMode(mode, name).waitForFinished();
}

void DisplayWorker::onMonitorListChanged(const QList<QDBusObjectPath> &mons)
{
    QList<QString> ops;
    for (const auto *mon : m_monitors.keys())
        ops << mon->path();

    QList<QString> pathList;
    for (const auto op : mons)
    {
        const QString path = op.path();
        pathList << path;
        if (!ops.contains(path))
            monitorAdded(path);
    }

    for (const auto op : ops)
        if (!pathList.contains(op))
            monitorRemoved(op);
}

void DisplayWorker::onMonitorsBrightnessChanged(const BrightnessMap &brightness)
{
    for (auto it(brightness.cbegin()); it != brightness.cend(); ++it)
        updateMonitorBrightness(it.key(), it.value());
}

void DisplayWorker::setMonitorRotate(Monitor *mon, const quint16 rotate)
{
    MonitorInter *inter = m_monitors.value(mon);
    Q_ASSERT(inter);

    inter->SetRotation(rotate).waitForFinished();
    m_displayInter.ApplyChanges();
}

void DisplayWorker::setPrimary(const int index)
{
    m_displayInter.SetPrimary(m_model->monitorList()[index]->name());
}

void DisplayWorker::setMonitorEnable(Monitor *mon, const bool enabled)
{
    MonitorInter *inter = m_monitors.value(mon);
    Q_ASSERT(inter);

    inter->Enable(enabled).waitForFinished();
    m_displayInter.ApplyChanges();
}

void DisplayWorker::setMonitorResolution(Monitor *mon, const int mode)
{
    MonitorInter *inter = m_monitors.value(mon);
    Q_ASSERT(inter);

    inter->SetMode(mode).waitForFinished();
    m_displayInter.ApplyChanges();
}

void DisplayWorker::setMonitorBrightness(Monitor *mon, const double brightness)
{
    m_displayInter.SetBrightness(mon->name(), std::max(brightness, 0.05)).waitForFinished();
}

void DisplayWorker::setMonitorPosition(Monitor *mon, const int x, const int y)
{
    MonitorInter *inter = m_monitors.value(mon);
    Q_ASSERT(inter);

    inter->SetPosition(x, y).waitForFinished();
    m_displayInter.ApplyChanges();
}

//void DisplayWorker::loadRotations(Monitor * const mon)
//{
//    MonitorInter *inter = m_monitors.value(mon);
//    Q_ASSERT(inter);

//    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(inter->ListRotations(), this);
//    connect(watcher, &QDBusPendingCallWatcher::finished, [=] (QDBusPendingCallWatcher *watcher) { loadRotationsFinished(mon, watcher); });
//}

//void DisplayWorker::loadRotationsFinished(Monitor * const mon, QDBusPendingCallWatcher *watcher)
//{
//    QDBusPendingReply<RotationList> reply = *watcher;
//    mon->setRotateList(reply.value());
//    watcher->deleteLater();
//}

//void DisplayWorker::loadModes(Monitor * const mon)
//{
//    MonitorInter *inter = m_monitors.value(mon);
//    Q_ASSERT(inter);

//    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(inter->ListModes(), this);
//    connect(watcher, &QDBusPendingCallWatcher::finished, [=] (QDBusPendingCallWatcher *watcher) { loadModesFinished(mon, watcher); });
//}

//void DisplayWorker::loadModesFinished(Monitor * const mon, QDBusPendingCallWatcher *watcher)
//{
//    QDBusPendingReply<ResolutionList> reply = *watcher;
//    mon->setModeList(reply.value());
//    watcher->deleteLater();
//}

void DisplayWorker::monitorAdded(const QString &path)
{
    MonitorInter *inter = new MonitorInter(DisplayInterface, path, QDBusConnection::sessionBus(), this);
    Monitor *mon = new Monitor(this);

    connect(inter, &MonitorInter::XChanged, mon, &Monitor::setX);
    connect(inter, &MonitorInter::YChanged, mon, &Monitor::setY);
    connect(inter, &MonitorInter::WidthChanged, mon, &Monitor::setW);
    connect(inter, &MonitorInter::HeightChanged, mon, &Monitor::setH);
    connect(inter, &MonitorInter::RotationChanged, mon, &Monitor::setRotate);
    connect(inter, &MonitorInter::NameChanged, mon, &Monitor::setName);
    connect(inter, &MonitorInter::CurrentModeChanged, mon, &Monitor::setCurrentMode);
    connect(inter, &MonitorInter::ModesChanged, mon, &Monitor::setModeList);
    connect(inter, &MonitorInter::RotationsChanged, mon, &Monitor::setRotateList);
    connect(&m_displayInter, static_cast<void (DisplayInter::*)(const QString &) const>(&DisplayInter::PrimaryChanged), mon, &Monitor::setPrimary);

    // NOTE: DO NOT using async dbus call. because we need to have a unique name to distinguish each monitor
    Q_ASSERT(inter->isValid());
    mon->setName(inter->name());

    inter->setSync(false);

    mon->setPath(path);
    mon->setX(inter->x());
    mon->setY(inter->y());
    mon->setW(inter->width());
    mon->setH(inter->height());
    mon->setRotate(inter->rotation());
    mon->setCurrentMode(inter->currentMode());
    mon->setModeList(inter->modes());
    mon->setRotateList(inter->rotations());
    mon->setPrimary(m_displayInter.primary());

    m_model->monitorAdded(mon);
    m_monitors.insert(mon, inter);
}

void DisplayWorker::monitorRemoved(const QString &path)
{
    Monitor *monitor = nullptr;
    for (auto it(m_monitors.cbegin()); it != m_monitors.cend(); ++it)
    {
        if (it.key()->path() == path)
        {
            monitor = it.key();
            break;
        }
    }
    if (!monitor)
        return;

    m_model->monitorRemoved(monitor);

    m_monitors[monitor]->deleteLater();
    m_monitors.remove(monitor);

    monitor->deleteLater();
}

void DisplayWorker::updateMonitorBrightness(const QString &monName, const double brightness)
{
    for (auto mon : m_monitors.keys())
    {
        if (mon->name() == monName)
        {
            mon->setBrightness(brightness);
            return;
        }
    }
}
