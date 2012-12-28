// Actions.
// Copyright (C) 2012 Gang Chen.

#include "actions.hpp"
#include "window.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

Actions::Actions(Window *window)
{
    m_actions = gtk_action_group_new("actions");
    gtk_action_group_set_translation_domain(m_actions, NULL);
    gtk_action_group_add_actions(m_actions,
                                 Actions::s_actionEntries,
                                 G_N_ELEMENTS(Actions::s_actionEntries),
                                 window);
}

Actions::~Actions()
{
    g_object_unref(m_actions);
}

void Actions::updateSensitivity(const Window *window)
{
}

}
