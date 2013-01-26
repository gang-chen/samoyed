// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_NOTEBOOK_HPP
#define SMYD_NOTEBOOK_HPP

#include "pane.hpp"
#include <vector>
#include <gtk/gtk.h>

namespace Samoyed
{

class Page;

class Notebook: public Pane
{
public:
    int pageCount() const { return m_pages.size(); }

    Page &page(int index) const { return *m_pages[index]; }

    void addPage(Page &page, int index);

    void removePage(Page &page);

    void onPageClosed();

    void onPageTitleChanged(Page &page);

    int currentPageIndex() const
    { return gtk_notebook_get_current_page(GTK_NOTEBOOK(m_notebook)); }

    void setCurrentPageIndex(int index)
    { gtk_notebook_set_current_page(GTK_NOTEBOOK(m_notebook), index); }

private:
    static void onCloseButtonClicked(GtkButton *button, gpointer page);

    std::vector<Page *> m_pages;

    GtkWidget *m_notebook;
};

}

#endif
