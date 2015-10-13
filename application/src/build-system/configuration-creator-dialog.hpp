// Configuration creator dialog.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_CONFIGURATION_CREATOR_DIALOG_HPP
#define SMYD_CONFIGURATION_CREATOR_DIALOG_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class Configuration;
class BuildSystem;

class ConfigurationCreatorDialog: public boost::noncopyable
{
public:
    ConfigurationCreatorDialog(GtkWindow *parent, BuildSystem &buildSystem);

    ~ConfigurationCreatorDialog();

    Configuration *run();

private:
    static void validateInput(gpointer object,
                              ConfigurationCreatorDialog *dialog);

    BuildSystem &m_buildSystem;

    GtkBuilder *m_builder;
    GtkDialog *m_dialog;
    GtkEntry *m_nameEntry;
    GtkEntry *m_configCommandsEntry;
    GtkEntry *m_buildCommandsEntry;
    GtkEntry *m_installCommandsEntry;
    GtkComboBox *m_compilerChooser;
    GtkToggleButton *m_autoCompilerOptions;
    GtkEntry *m_compilerOptionsEntry;
};

}

#endif
