#include "zenmodepluginconstants.h"
#include "zenmodeplugin.h"
#include "zenmodeplugintr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <QAction>
#include <QMenu>

using namespace Core;

namespace ZenModePlugin::Internal {

void printAvailableCommandsIDs()
{
    QFile outputFile("command_ids_list.log");
    outputFile.open(QIODeviceBase::Truncate | QIODeviceBase::WriteOnly);
    QTextStream outTextStr(&outputFile);

    QList<Core::Command*> allCommands = Core::ActionManager::commands();
    // Iterate through all commands and print their IDs
    for (Core::Command* cmd : allCommands) {
        QString actionString;
        outTextStr << "Command ID '" << cmd->id().toString()
                   << "', Description '" << cmd->description()
                   << "', has Action '" << (cmd->action() != nullptr)
                   << "', action check state'" << ((cmd->action() != nullptr) ? (cmd->action()->isChecked() ? "CHECKED" : "UNCHECKED") :"null")
                   << "'\n";
        if (QAction* action = cmd->action()) {
            actionString += QString("action %1").arg(cmd->action()->isChecked() ? "CHECKED" : "UNCHECKED");
        }

        QString msg = QString("=> Command ID: %1, Description: %2").arg(cmd->id().toString()).arg(cmd->description());
        qDebug() << msg << actionString;
    }
    outputFile.close();

    QObject::connect(Core::ActionManager::instance(), &Core::ActionManager::commandAdded, [](Utils::Id id) {
        QFile outputFile("command_ids_list.log");
        outputFile.open(QIODeviceBase::Append);
        QTextStream outTextStr(&outputFile);
        if (const Core::Command* cmd = Core::ActionManager::command(id)) {
            outTextStr << "Command Added - ID '" << cmd->id().toString()
                       << "', Description '" << cmd->description()
                       << "', has Action '" << (cmd->action() != nullptr)
                       << "', action check state'" << ((cmd->action() != nullptr) ? (cmd->action()->isChecked() ? "CHECKED" : "UNCHECKED") :"null")
                   << "'\n";
        }
        outputFile.close();
    });
}


ZenModePluginCore::~ZenModePluginCore()
{ }

void ZenModePluginCore::initialize()
{
    ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(Tr::tr("Zen Mode"));
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    ActionBuilder(this, Constants::DISTRACTION_FREE_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Toogle Distraction Free Mode"))
        .setDefaultKeySequence(Tr::tr("Shift+Escape"))
        .addOnTriggered(this, &ZenModePluginCore::toggleDistractionFreeMode);
}

void ZenModePluginCore::extensionsInitialized()
{ }

ZenModePluginCore::ShutdownFlag ZenModePluginCore::aboutToShutdown()
{
    return SynchronousShutdown;
}

void ZenModePluginCore::toggleDistractionFreeMode()
{
    m_distractionFreeModeActive = !m_distractionFreeModeActive;
}

} // namespace ZenModePlugin::Internal

#include <zenmodeplugin.moc>
