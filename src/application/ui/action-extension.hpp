// Extension: action.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_ACTION_EXTENSION_HPP
#define SMYD_ACTION_EXTENSION_HPP

#include "utilities/extension.hpp"

namespace Samoyed
{

class Window;

class ActionExtension: public Extension
{
public:
    ActionExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual void setAction(Window &window, GtkAction *action) = 0;

    virtual void setToggleAction(Window &window, GtkToggleAction *action) = 0;

    virtual void activateAction(Window &window, GtkAction *action) = 0;

    virtual void onActionToggled(Window &window, GtkToggleAction *action) = 0;

    virtual bool isActionSensitive(Window &window, GtkAction *action) = 0;
};

}

#endif
