#include "zenmodepluginconstants.h"
#include "zenmodeplugin.h"
#include "zenmodeplugintr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditor.h>
#include <texteditor/marginsettings.h>

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

const Utils::Id LEFT_SIDEBAR_COMMAND_ID{"QtCreator.ToggleLeftSidebar"};
const Utils::Id RIGHT_SIDEBAR_COMMAND_ID{"QtCreator.ToggleRightSidebar"};
const Utils::Id OUTPUT_PANE_COMMAND_ID{"QtCreator.Pane.GeneralMessages"};
const Utils::Id FULLSCREEN_COMMAND_ID("QtCreator.ToggleFullScreen");

const ZenModePluginCore::ModeStyle MODES_STATE_ON_ACTIVE_ZENMODE{ZenModePluginCore::ModeStyle::Hidden};

const std::vector<Utils::Id> TOGGLE_MODES_STATES_COMMANDS = {
    "QtCreator.Modes.Hidden",
    "QtCreator.Modes.IconsOnly",
    "QtCreator.Modes.IconsAndText"
};

class CustomTextEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT
public:
    explicit CustomTextEditorWidget(QWidget *parent = nullptr)
        : TextEditor::TextEditorWidget(parent)
    {
    }

    // Public method to set custom extra area width
    void setCustomExtraAreaWidth(int width)
    {
        m_customExtraAreaWidth = width;
        // return TextEditor::TextEditorWidget::extraAreaWidth(markWidthPtr);
        //updateExtraAreaWidth();
        // int ea_width = this->extraAreaWidth();

        // Update viewport margins - this makes the text editor shift right
        // setViewportMargins(ea_width, 0, 0, 0);
        setViewportMargins(width, 0, 0, 0);

        // // Update extra area geometry to match
        // QWidget *ea = extraArea();
        // if (ea) {
        //     QRect cr = contentsRect();
        //     // ea->setGeometry(QRect(cr.left(), cr.top(), ea_width, cr.height()));
        //     ea->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));
        // }
    }

private:
    int m_customExtraAreaWidth = 0;

    // Override to use custom width
    virtual int extraAreaWidth(int *markWidthPtr = nullptr) const override
    {
        if (m_customExtraAreaWidth > 0) {
            return m_customExtraAreaWidth;
        }
        return TextEditor::TextEditorWidget::extraAreaWidth(markWidthPtr);
    }

    // Method to update viewport margins and extra area geometry
    void updateExtraAreaWidth()
    {
    }

protected:
    // Override resizeEvent to keep extra area synchronized
    void resizeEvent(QResizeEvent *e) override
    {
        TextEditor::TextEditorWidget::resizeEvent(e);

        if (m_customExtraAreaWidth > 0) {
            int width = extraAreaWidth();
            QRect cr = contentsRect();
            QWidget *ea = extraArea();
            if (ea) {
                ea->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));
            }
        }
    }
};

class TextEditorWidgetAccessor : public TextEditor::TextEditorWidget
{
public:
    static void setMargins(TextEditor::TextEditorWidget *widget, int left, int top, int right, int bottom)
    {
        // Cast to accessor to get access to protected method
        TextEditorWidgetAccessor *accessor = static_cast<TextEditorWidgetAccessor*>(widget);
        accessor->setViewportMargins(left, top, right, bottom);
    }

    static void updateExtraAreaGeometry(TextEditor::TextEditorWidget *widget, int width)
    {
        TextEditorWidgetAccessor *accessor = static_cast<TextEditorWidgetAccessor*>(widget);

        // Update viewport margins
        auto margins = accessor->viewportMargins();
        qDebug() << "=> updateExtraAreaGeometry " << width << ", margins " << margins << " before";
        accessor->setViewportMargins(width, 0, 0, 0);
        auto margins2 = accessor->viewportMargins();
        qDebug() << "=> updateExtraAreaGeometry viewport contentMargins" <<
            widget->viewport()->contentsMargins() << ",  margins " <<
            margins2 << ",  after";

        // Update extra area geometry
        QWidget *extraArea = widget->extraArea();
        if (extraArea) {
            QRect cr = widget->contentsRect();
            extraArea->setGeometry(QRect(cr.left(), cr.top(), width, cr.height()));
        }
    }
};

// Usage in your plugin code
void setCustomExtraAreaWidth(TextEditor::TextEditorWidget *editorWidget, int width)
{
    TextEditorWidgetAccessor::updateExtraAreaGeometry(editorWidget, width);
}

ZenModePluginCore::~ZenModePluginCore()
{ }

void ZenModePluginCore::initialize()
{
    ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(Tr::tr("Zen Mode"));
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    ActionBuilder(this, Constants::ZEN_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Toogle Zen Mode"))
        .setDefaultKeySequence(Tr::tr("Shift+Alt+Z"))
        .addOnTriggered(this, &ZenModePluginCore::triggerAction);

    ActionBuilder(this, Constants::HIDE_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Hide All Tool Panes"))
        .setDefaultKeySequence(Tr::tr("Shift+Escape"))
        .addOnTriggered(this, &ZenModePluginCore::hideOutputPanes);
}

void ZenModePluginCore::extensionsInitialized()
{ }

bool ZenModePluginCore::delayedInitialize()
{
    getActions();

    m_window = Core::ICore::mainWindow();
    fullScreenMode(false);
    return true;
}

ZenModePluginCore::ShutdownFlag ZenModePluginCore::aboutToShutdown()
{
    fullScreenMode(false);
    restoreModeSidebar();
    restoreSidebars();
    return SynchronousShutdown;
}

void ZenModePluginCore::getActions()
{
    if (const Core::Command* cmd = Core::ActionManager::command(OUTPUT_PANE_COMMAND_ID))
    {
        m_outputPaneAction = cmd->action();
    } else {
        qWarning() << "ZenModePlugin - fail to get" <<  OUTPUT_PANE_COMMAND_ID.toString() << "action";
    }

    if (const Core::Command* cmd = Core::ActionManager::command(LEFT_SIDEBAR_COMMAND_ID)) {
        m_toggleLeftSidebarAction = cmd->action();
    } else {
        qWarning() << "ZenModePlugin - fail to get" <<  LEFT_SIDEBAR_COMMAND_ID.toString() << "action";
    }

    if (const Core::Command* cmd = Core::ActionManager::command(RIGHT_SIDEBAR_COMMAND_ID))
    {
        m_toggleRightSidebarAction = cmd->action();
    } else {
        qWarning() << "ZenModePlugin - fail to get" <<  RIGHT_SIDEBAR_COMMAND_ID.toString() << "action";
    }

    m_toggleModesStatesActions.resize(3);
    for (int i = 0; i < TOGGLE_MODES_STATES_COMMANDS.size(); i++)
    {
        if (const Core::Command* cmd = Core::ActionManager::command(TOGGLE_MODES_STATES_COMMANDS[i]))
        {
            m_toggleModesStatesActions[i] = cmd->action();
            if (cmd->action()->isChecked())
            {
                m_prevModesSidebarState = (ModeStyle)i;
            }
        }
    }

    if (const Core::Command* cmd = Core::ActionManager::command(FULLSCREEN_COMMAND_ID))
    {
        m_toggleFullscreenAction = cmd->action();
    } else {
        qWarning() << "ZenModePlugin - fail to get" <<  FULLSCREEN_COMMAND_ID.toString() << "action";
    }
}

void ZenModePluginCore::triggerTextEditorTextCenter()
{
    // Get the current editor
    Core::IEditor *currentEditor = Core::EditorManager::currentEditor();
    qDebug() << "=> currentEditor " << currentEditor;
    if (currentEditor) {
        // Cast to text editor
        auto *textEditor = qobject_cast<TextEditor::BaseTextEditor*>(currentEditor);

        qDebug() << "=> textEditor " << textEditor;
        if (textEditor) {
            TextEditor::TextEditorWidget * editorWidget = textEditor->editorWidget();
            auto *qPlainTestEdit = qobject_cast<QPlainTextEdit*>(textEditor->editorWidget());
            auto *PlainTextEdit  = qobject_cast<Utils::PlainTextEdit*>(editorWidget);
            qDebug() << "=> editorWidget " << editorWidget;

            if (editorWidget) {
                // Cast to your custom widget type
                CustomTextEditorWidget *customWidget = qobject_cast<CustomTextEditorWidget*>(textEditor->editorWidget());
                CustomTextEditorWidget *customWidget1 = static_cast<CustomTextEditorWidget*>(textEditor->editorWidget());

                QWidget * extraAreaWidget = editorWidget->extraArea();
                // if (m_active)
                {
                    if (m_active)
                    {

                        m_prevExtraAreaWidth = extraAreaWidget->width();
                        setCustomExtraAreaWidth(editorWidget, 300);
                        // extraAreaWidget->setFixedWidth(200);
                        // m_prevMargin = editorWidget->document()->documentMargin();
                        // editorWidget->document()->setDocumentMargin(0);
                        // qInfo() << "=> prev margin " << m_prevMargin;
                    } else {
                        // if (customWidget) {
                        //     customWidget->setCustomExtraAreaWidth(300); // Set to 200 or 300 as needed
                        // }
                        setCustomExtraAreaWidth(editorWidget, std::max(m_prevExtraAreaWidth, 0));
                        // editorWidget->document()->setDocumentMargin(m_prevMargin);
                    }
                    // QMargins orgMargin = editorWidget->viewportMargins();

                    // Apply centered margins
                    // int margin = 200; // Or calculate dynamically
                    // editorWidget->setViewportMargins(margin, 0, margin, 0);
                    // qInfo() << "=> triggering TextEditor margins " << orgMargin;
                    // editorWidget->setViewportMargins(orgMargin.left(), orgMargin.top(), orgMargin.right(), orgMargin.bottom());
                }
            }
        }
    }
}

void ZenModePluginCore::hideOutputPanes()
{
    if (m_outputPaneAction)
    {
        m_outputPaneAction->trigger();
        m_outputPaneAction->trigger();
    }
}

void ZenModePluginCore::hideSidebars()
{
    if (m_toggleLeftSidebarAction)
    {
        m_prevLeftSidebarState = m_toggleLeftSidebarAction->isChecked();
       if (m_prevLeftSidebarState)
       {
           m_toggleLeftSidebarAction->trigger();
       }
    }

    if (m_toggleRightSidebarAction)
    {
        m_prevRightSidebarState = m_toggleRightSidebarAction->isChecked();
        if(m_prevRightSidebarState)
        {
            m_toggleRightSidebarAction->trigger();
        }
    }
}

void ZenModePluginCore::restoreSidebars()
{
    if (m_toggleLeftSidebarAction &&
        !m_toggleLeftSidebarAction->isChecked() &&
        m_prevLeftSidebarState)
    {
        m_prevLeftSidebarState = false;
        m_toggleLeftSidebarAction->trigger();
    }
    if (m_toggleRightSidebarAction &&
        !m_toggleRightSidebarAction->isChecked() &&
        m_prevRightSidebarState )
    {
        m_prevRightSidebarState = false;
        m_toggleRightSidebarAction->trigger();
    }
}

void ZenModePluginCore::hideModeSidebar()
{
    for (int i = 0; i < m_toggleModesStatesActions.size(); i++)
    {
        auto action = m_toggleModesStatesActions[i];
            qDebug(QString("=> hideModeSidebar mode (%1), hasAction %2, checkState %3").arg(i)
                       .arg(action != nullptr)
                       // .arg(action->isChecked())
                       .arg(false)
                       .toLatin1());
        if (action && action->isChecked())
        {
            m_prevModesSidebarState = (ModeStyle)i;
        }
    }
    auto action = m_toggleModesStatesActions[MODES_STATE_ON_ACTIVE_ZENMODE];
    if (action && !action->isChecked())
    {
        action->trigger();
    }
}

void ZenModePluginCore::restoreModeSidebar()
{
    if (m_prevModesSidebarState >= ModeStyle::Hidden && m_prevModesSidebarState <= ModeStyle::IconsAndText)
    {
        auto action = m_toggleModesStatesActions[m_prevModesSidebarState];
        if (action && !action->isChecked()) {
            action->trigger();
        }
    }
}

void ZenModePluginCore::fullScreenMode(bool state)
{
    if (m_toggleFullscreenAction && state != m_window->isFullScreen())
    {
        m_toggleFullscreenAction->trigger();
    }
}

void ZenModePluginCore::triggerAction()
{
    m_active = !m_active;
    qDebug() << "ZenMode::triggerAction - state" << (m_active ? "ACTIVE" : "INACTIVE") ;
    if (m_active)
    {
        hideOutputPanes();
        hideSidebars();
        hideModeSidebar();
        fullScreenMode(true);
    } else {
        restoreSidebars();
        restoreModeSidebar();
        fullScreenMode(false);
    }
    // triggerTextEditorTextCenter();
}
} // namespace ZenModePlugin::Internal

#include <zenmodeplugin.moc>
