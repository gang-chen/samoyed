// Paned widget.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PANED_HPP
#define SMYD_PANED_HPP

#include "widget-container.hpp"
#include <list>
#include <string>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class Paned: public WidgetContainer
{
public:
    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    class XmlElement: public WidgetContainer::XmlElement
    {
    public:
        static void registerReader();

        virtual ~XmlElement();
        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const Paned &paned);
        virtual Widget *restoreWidget();

        Paned::Orientation orientation() const { return m_orientation; }
        int sidePaneIndex() const { return m_sidePaneIndex; }
        Widget::XmlElement &child(int index) const
        { return *m_children[index]; }
        int currentChildIndex() const { return m_currentChildIndex; }
        double child1SizeFraction() const
        { return m_child1SizeFraction; }
        int sidePaneSize() const { return m_sidePaneSize; }

    protected:
        XmlElement():
            m_orientation(Paned::ORIENTATION_HORIZONTAL),
            m_sidePaneIndex(-1),
            m_currentChildIndex(0),
            m_child1SizeFraction(0.5)
        {
            m_children[0] = NULL;
            m_children[1] = NULL;
        }

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
        Paned::Orientation m_orientation;
        int m_sidePaneIndex;
        Widget::XmlElement *m_children[2];
        int m_currentChildIndex;
        double m_child1SizeFraction;
        int m_sidePaneSize;
    };

    static Paned *create(const char *id,
                         Orientation orientation,
                         Widget &child1, Widget &child2,
                         double child1SizeFraction);

    static Paned *create(const char *id,
                         Orientation orientation,
                         int sidePaneIndex,
                         Widget &child1, Widget &child2,
                         int sidePaneSize);

    static Paned *split(const char *id,
                        Orientation orientation,
                        Widget &child1, Widget &child2,
                        double child1SizeFraction);

    static Paned *split(const char *id,
                        Orientation orientation,
                        int sidePaneIndex,
                        Widget &child1, Widget &child2,
                        int sidePaneSize);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void removeChild(Widget &child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const { return 2; }

    virtual Widget &child(int index) { return *m_children[index]; }
    virtual const Widget &child(int index) const { return *m_children[index]; }

    virtual int currentChildIndex() const { return m_currentChildIndex; }
    virtual void setCurrentChildIndex(int index)
    { m_currentChildIndex = index; }

    Orientation orientation() const
    {
        return static_cast<Orientation>(
            gtk_orientable_get_orientation(GTK_ORIENTABLE(gtkWidget())));
    }

    int sidePaneIndex() const { return m_sidePaneIndex; }

    double child1SizeFraction() const;
    void setChild1SizeFraction(double fraction);

    int sidePaneSize() const;
    void setSidePaneSize(int size);

protected:
    Paned():
        m_currentChildIndex(0),
        m_child1SizeFractionRequest(-1.),
        m_sidePaneSizeRequest(-1)
    {
        m_children[0] = NULL;
        m_children[1] = NULL;
    }

    virtual ~Paned();

    bool setup(const char *id,
               Orientation orientation,
               int sidePaneIndex,
               Widget &child1, Widget &child2);

    bool restore(XmlElement &xmlElement);

    void addChildInternally(Widget &child, int index);

    void removeChildInternally(Widget &child);

private:
    static void setFocusChild(GtkWidget *container,
                              GtkWidget *child,
                              Paned *paned);

    void setPositionInternally();
    static void setPositionOnMapped(GtkWidget *widget, Paned *paned);

    Widget *m_children[2];

    int m_currentChildIndex;
    int m_sidePaneIndex;
    double m_child1SizeFractionRequest;
    int m_sidePaneSizeRequest;
};

}

#endif
