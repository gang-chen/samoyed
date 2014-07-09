// Action extension: find text.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_FIND_FIND_TEXT_ACTION_EXTENSION_HPP
#define SMYD_FIND_FIND_TEXT_ACTION_EXTENSION_HPP

namespace Samoyed
{

namespace Finder
{

class FindTextActionExtension: public ActionExtension
{
public:
    FindTextActionExtension(const char *id, Plugin &plugin):
        ActionExtension(id, plugin)
    {}

    virtual void addAction(Window &window, GtkAction *action) {}

    virtual void addToggleAction(Window &window, GtkToggleAction *action) {}

    virtual void activateAction(Window &window, GtkAction *action);

    virtual void onActionToggled(Window &window, GtkToggleAction *action) {}

    virtual bool isActionSensitive(Window &window, GtkAction *action);
};

}

}

#endif
