// Paned widget.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "paned.hpp"
#include <assert.h>
#include <string.h>
#include <list>
#include <string>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define WIDGET_CONTAINER "widget-container"
#define PANED "paned"
#define ORIENTATION "orientation"
#define HORIZONTAL "horizontal"
#define VERTICAL "vertical"
#define SIDE_PANE_INDEX "side-pane-index"
#define CHILDREN "children"
#define CURRENT_CHILD_INDEX "current-child-index"
#define CHILD1_SIZE_FRACTION "child1-size-fraction"
#define SIDE_PANE_SIZE "side-pane-size"

namespace Samoyed
{

void Paned::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(PANED,
                                       Widget::XmlElement::Reader(read));
}

bool Paned::XmlElement::readInternally(xmlNodePtr node,
                                       std::list<std::string> *errors)
{
    char *value, *cp;
    bool containerSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET_CONTAINER) == 0)
        {
            if (containerSeen)
            {
                if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one \"%s\" elements seen.\n"),
                        child->line, WIDGET_CONTAINER);
                    errors->push_back(cp);
                    g_free(cp);
                }
                return false;
            }
            if (!WidgetContainer::XmlElement::readInternally(child, errors))
                return false;
            containerSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ORIENTATION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                if (strcmp(value, HORIZONTAL) == 0)
                    m_orientation = Paned::ORIENTATION_HORIZONTAL;
                else if (strcmp(value, VERTICAL) == 0)
                    m_orientation = Paned::ORIENTATION_VERTICAL;
                else if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid orientation \"%s\".\n"),
                        child->line, value);
                    errors->push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        SIDE_PANE_INDEX) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_sidePaneIndex = boost::lexical_cast<double>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, SIDE_PANE_INDEX, exp.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
                if (m_sidePaneIndex < -1 || m_sidePaneIndex > 1)
                    m_sidePaneIndex = -1;
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CHILDREN) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                Widget::XmlElement *ch =
                    Widget::XmlElement::read(grandChild, errors);
                if (ch)
                {
                    if (m_children[1])
                    {
                        if (errors)
                        {
                            cp = g_strdup_printf(
                                _("Line %d: More than two children contained "
                                  "by the paned widget.\n"),
                                grandChild->line);
                            errors->push_back(cp);
                            g_free(cp);
                        }
                        delete ch;
                    }
                    else if (m_children[0])
                        m_children[1] = ch;
                    else
                        m_children[0] = ch;
                }
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CURRENT_CHILD_INDEX) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_currentChildIndex = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, CURRENT_CHILD_INDEX,
                            exp.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CHILD1_SIZE_FRACTION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_child1SizeFraction = boost::lexical_cast<double>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid real number \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, CHILD1_SIZE_FRACTION,
                            exp.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        SIDE_PANE_SIZE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_sidePaneSize = boost::lexical_cast<double>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, SIDE_PANE_SIZE, exp.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
    }

    if (!containerSeen)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, WIDGET_CONTAINER);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    if (!m_children[0])
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: No child contained by the paned widget.\n"),
                node->line);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    if (!m_children[1])
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: Only one child contained by the paned widget.\n"),
                node->line);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }

    if (m_currentChildIndex < 0)
        m_currentChildIndex = 0;
    else if (m_currentChildIndex > 1)
        m_currentChildIndex = 1;
    return true;
}

Paned::XmlElement *Paned::XmlElement::read(xmlNodePtr node,
                                           std::list<std::string> *errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr Paned::XmlElement::write() const
{
    std::string str;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(PANED));
    xmlAddChild(node, WidgetContainer::XmlElement::write());
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(ORIENTATION),
                    m_orientation ==
                    Paned::ORIENTATION_HORIZONTAL ?
                    reinterpret_cast<const xmlChar *>(HORIZONTAL) :
                    reinterpret_cast<const xmlChar *>(VERTICAL));
    str = boost::lexical_cast<std::string>(m_sidePaneIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(SIDE_PANE_INDEX),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    xmlNodePtr children = xmlNewNode(NULL,
                                     reinterpret_cast<const xmlChar *>(CHILDREN));
    xmlAddChild(children, m_children[0]->write());
    xmlAddChild(children, m_children[1]->write());
    xmlAddChild(node, children);
    str = boost::lexical_cast<std::string>(m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURRENT_CHILD_INDEX),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    if (m_sidePaneIndex == -1)
    {
        str = boost::lexical_cast<std::string>(m_child1SizeFraction);
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(CHILD1_SIZE_FRACTION),
                        reinterpret_cast<const xmlChar *>(str.c_str()));
    }
    else
    {
        str = boost::lexical_cast<std::string>(m_sidePaneSize);
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(SIDE_PANE_SIZE),
                        reinterpret_cast<const xmlChar *>(str.c_str()));
    }
    return node;
}

Paned::XmlElement::XmlElement(const Paned &paned):
    WidgetContainer::XmlElement(paned)
{
    m_orientation = paned.orientation();
    m_sidePaneIndex = paned.sidePaneIndex();
    m_children[0] = paned.child(0).save();
    m_children[1] = paned.child(1).save();
    m_currentChildIndex = paned.currentChildIndex();
    if (m_sidePaneIndex == -1)
        m_child1SizeFraction = paned.child1SizeFraction();
    else
        m_sidePaneSize = paned.sidePaneSize();
}

Widget *Paned::XmlElement::restoreWidget()
{
    Paned *paned = new Paned;
    if (!paned->restore(*this))
    {
        delete paned;
        return NULL;
    }
    return paned;
}

Paned::XmlElement::~XmlElement()
{
    delete m_children[0];
    delete m_children[1];
}

bool Paned::setup(const char *id,
                  Orientation orientation,
                  int sidePaneIndex,
                  Widget &child1, Widget &child2)
{
    if (!WidgetContainer::setup(id))
        return false;
    GtkWidget *paned = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    g_signal_connect(paned, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(paned);
    gtk_widget_show_all(paned);
    m_sidePaneIndex = sidePaneIndex;
    addChildInternally(child1, 0);
    addChildInternally(child2, 1);
    return true;
}

Paned *Paned::create(const char *id,
                     Orientation orientation,
                     Widget &child1, Widget &child2,
                     double child1SizeFraction)
{
    Paned *paned = new Paned;
    if (!paned->setup(id, orientation, -1, child1, child2))
    {
        delete paned;
        return NULL;
    }
    paned->setChild1SizeFraction(child1SizeFraction);
    return paned;
}

Paned *Paned::create(const char *id,
                     Orientation orientation,
                     int sidePaneIndex,
                     Widget &child1, Widget &child2,
                     int sidePaneSize)
{
    Paned *paned = new Paned;
    if (!paned->setup(id, orientation, sidePaneIndex, child1, child2))
    {
        delete paned;
        return NULL;
    }
    paned->setSidePaneSize(sidePaneSize);
    return paned;
}

bool Paned::restore(XmlElement &xmlElement)
{
    if (!WidgetContainer::restore(xmlElement))
        return false;
    Widget *child1 = xmlElement.child(0).restoreWidget();
    if (!child1)
        return false;
    Widget *child2 = xmlElement.child(1).restoreWidget();
    if (!child2)
    {
        // Assume that it should close itself silently.
        child1->close();
        return false;
    }
    GtkWidget *paned =
        gtk_paned_new(static_cast<GtkOrientation>(xmlElement.orientation()));
    g_signal_connect(paned, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(paned);
    if (xmlElement.visible())
        gtk_widget_show_all(paned);
    m_sidePaneIndex = xmlElement.sidePaneIndex();
    addChildInternally(*child1, 0);
    addChildInternally(*child2, 1);
    setCurrentChildIndex(xmlElement.currentChildIndex());
    if (m_sidePaneIndex == -1)
        setChild1SizeFraction(xmlElement.child1SizeFraction());
    else
        setSidePaneSize(xmlElement.sidePaneSize());
    return true;
}

Paned::~Paned()
{
    assert(!m_children[0] && !m_children[1]);
}

bool Paned::close()
{
    if (closing())
        return true;

    setClosing(true);
    Widget *child1 = m_children[m_currentChildIndex];
    Widget *child2 = m_children[1 - m_currentChildIndex];
    if (!child1->close())
    {
        setClosing(false);
        return false;
    }
    if (!child2->close())
        // Do not call setClosing(false) because the paned widget will be
        // deleted if either child is closed.
        return false;
    return true;
}

Widget::XmlElement *Paned::save() const
{
    return new XmlElement(*this);
}

void Paned::addChildInternally(Widget &child, int index)
{
    assert(!m_children[index]);
    WidgetContainer::addChildInternally(child);
    m_children[index] = &child;
    if (index == 0)
        gtk_paned_pack1(GTK_PANED(gtkWidget()), child.gtkWidget(),
                        index != m_sidePaneIndex, TRUE);
    else
        gtk_paned_pack2(GTK_PANED(gtkWidget()), child.gtkWidget(),
                        index != m_sidePaneIndex, TRUE);
}

void Paned::removeChildInternally(Widget &child)
{
    int index = childIndex(child);
    m_children[index] = NULL;
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(gtkWidget()), child.gtkWidget());
    WidgetContainer::removeChildInternally(child);
}

void Paned::removeChild(Widget &child)
{
    int index = childIndex(child);
    removeChildInternally(child);

    // Remove the remained child from this paned widget.
    Widget *remained = m_children[1 - index];
    assert(remained);
    removeChildInternally(*remained);

    // Replace this paned widget with the remained child.
    assert(parent());
    parent()->replaceChild(*this, *remained);

    // Destroy this paned widget.
    destroyInternally();
}

void Paned::replaceChild(Widget &oldChild, Widget &newChild)
{
    int index = childIndex(oldChild);
    removeChildInternally(oldChild);
    addChildInternally(newChild, index);
}

Paned *Paned::split(const char *id,
                    Orientation orientation,
                    Widget &child1, Widget &child2,
                    double child1SizeFraction)
{
    Widget *original;
    WidgetContainer *parent;
    int newChildIndex;

    if (child1.parent())
    {
        original = &child1;
        newChildIndex = 1;
    }
    else
    {
        assert(child2.parent());
        original = &child2;
        newChildIndex = 0;
    }
    parent = original->parent();

    // If the original widget is mapped, save its size and set the position of
    // the new paned widget.  Note that if the original widget is mapped, then
    // the paned widget will be mapped after replacing the original one, but its
    // size will not be allocated.
    int totalSize = -1;
    if (gtk_widget_get_mapped(original->gtkWidget()))
    {
        if (orientation == ORIENTATION_HORIZONTAL)
            totalSize = gtk_widget_get_allocated_width(original->gtkWidget());
        else
            totalSize = gtk_widget_get_allocated_height(original->gtkWidget());
    }

    // Create a paned widget to hold the two widgets.
    Paned *paned = new Paned;
    if (!paned->WidgetContainer::setup(id))
    {
        delete paned;
        return NULL;
    }
    GtkWidget *p = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    g_signal_connect(p, "set-focus-child",
                     G_CALLBACK(setFocusChild), paned);
    paned->setGtkWidget(p);
    paned->m_sidePaneIndex = -1;

    // Replace the original widget with the paned widget.
    parent->replaceChild(*original, *paned);

    // Add the two widgets to the paned widget.
    paned->addChildInternally(child1, 0);
    paned->addChildInternally(child2, 1);
    paned->m_currentChildIndex = newChildIndex;

    gtk_widget_show_all(p);

    if (totalSize != -1)
    {
        GValue handleSize = G_VALUE_INIT;
        g_value_init(&handleSize, G_TYPE_INT);
        gtk_widget_style_get_property(paned->gtkWidget(),
                                      "handle-size", &handleSize);
        gtk_paned_set_position(GTK_PANED(paned->gtkWidget()),
                               (totalSize - g_value_get_int(&handleSize)) *
                               child1SizeFraction);
    }
    else
        paned->setChild1SizeFraction(child1SizeFraction);

    return paned;
}

Paned *Paned::split(const char *id,
                    Orientation orientation,
                    int sidePaneIndex,
                    Widget &child1, Widget &child2,
                    int sidePaneSize)
{
    Widget *original;
    WidgetContainer *parent;
    int newChildIndex;

    if (child1.parent())
    {
        original = &child1;
        newChildIndex = 1;
    }
    else
    {
        assert(child2.parent());
        original = &child2;
        newChildIndex = 0;
    }
    parent = original->parent();

    // If the original widget is mapped, save its size and set the position of
    // the new paned widget.  Note that if the original widget is mapped, then
    // the paned widget will be mapped after replacing the original one, but its
    // size will not be allocated.
    int totalSize = -1;
    if (gtk_widget_get_mapped(original->gtkWidget()))
    {
        if (orientation == ORIENTATION_HORIZONTAL)
            totalSize = gtk_widget_get_allocated_width(original->gtkWidget());
        else
            totalSize = gtk_widget_get_allocated_height(original->gtkWidget());
    }

    // Create a paned widget to hold the two widgets.
    Paned *paned = new Paned;
    if (!paned->WidgetContainer::setup(id))
    {
        delete paned;
        return NULL;
    }
    GtkWidget *p = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    g_signal_connect(p, "set-focus-child",
                     G_CALLBACK(setFocusChild), paned);
    paned->setGtkWidget(p);
    paned->m_sidePaneIndex = sidePaneIndex;

    // Replace the original widget with the paned widget.
    parent->replaceChild(*original, *paned);

    // Add the two widgets to the paned widget.
    paned->addChildInternally(child1, 0);
    paned->addChildInternally(child2, 1);
    paned->m_currentChildIndex = newChildIndex;

    gtk_widget_show_all(p);

    if (totalSize != -1)
    {
        GValue handleSize = G_VALUE_INIT;
        g_value_init(&handleSize, G_TYPE_INT);
        gtk_widget_style_get_property(paned->gtkWidget(),
                                      "handle-size", &handleSize);
        if (sidePaneIndex == 0)
            gtk_paned_set_position(GTK_PANED(paned->gtkWidget()),
                                   sidePaneSize);
        else
            gtk_paned_set_position(GTK_PANED(paned->gtkWidget()),
                                   totalSize - g_value_get_int(&handleSize) -
                                   sidePaneSize);
    }
    else
        paned->setSidePaneSize(sidePaneSize);

    return paned;
}

void Paned::setFocusChild(GtkWidget *container,
                          GtkWidget *child,
                          Paned *paned)
{
    for (int i = 0; i < paned->childCount(); ++i)
        if (paned->m_children[i] && paned->m_children[i]->gtkWidget() == child)
        {
            paned->setCurrentChildIndex(i);
            break;
        }
}

void Paned::setPositionInternally()
{
    int totalSize;
    GValue handleSize = G_VALUE_INIT;
    if (gtk_orientable_get_orientation(GTK_ORIENTABLE(gtkWidget())) ==
        GTK_ORIENTATION_HORIZONTAL)
        totalSize = gtk_widget_get_allocated_width(gtkWidget());
    else
        totalSize = gtk_widget_get_allocated_height(gtkWidget());
    g_value_init(&handleSize, G_TYPE_INT);
    gtk_widget_style_get_property(gtkWidget(), "handle-size", &handleSize);
    if (m_sidePaneIndex == -1)
    {
        gtk_paned_set_position(GTK_PANED(gtkWidget()),
                               (totalSize - g_value_get_int(&handleSize)) *
                               m_child1SizeFractionRequest);
        m_child1SizeFractionRequest = -1.;
    }
    else if (m_sidePaneIndex == 0)
    {
        gtk_paned_set_position(GTK_PANED(gtkWidget()), m_sidePaneSizeRequest);
        m_sidePaneSizeRequest = -1;
    }
    else
    {
        assert(m_sidePaneIndex == 1);
        gtk_paned_set_position(GTK_PANED(gtkWidget()),
                               totalSize - g_value_get_int(&handleSize) -
                               m_sidePaneSizeRequest);
        m_sidePaneSizeRequest = -1;
    }
}

void Paned::setPositionOnMapped(GtkWidget *widget, Paned *paned)
{
    paned->setPositionInternally();
    g_signal_handlers_disconnect_by_func(
        widget,
        reinterpret_cast<gpointer>(setPositionOnMapped),
        paned);
}

double Paned::child1SizeFraction() const
{
    if (m_child1SizeFractionRequest > 0. && m_child1SizeFractionRequest < 1.)
        return m_child1SizeFractionRequest;
    int totalSize;
    GValue handleSize = G_VALUE_INIT;
    if (gtk_orientable_get_orientation(GTK_ORIENTABLE(gtkWidget())) ==
        GTK_ORIENTATION_HORIZONTAL)
        totalSize = gtk_widget_get_allocated_width(gtkWidget());
    else
        totalSize = gtk_widget_get_allocated_height(gtkWidget());
    g_value_init(&handleSize, G_TYPE_INT);
    gtk_widget_style_get_property(gtkWidget(), "handle-size", &handleSize);
    return
        (static_cast<double>(gtk_paned_get_position(GTK_PANED(gtkWidget()))) /
         (totalSize - g_value_get_int(&handleSize)));
}

void Paned::setChild1SizeFraction(double fraction)
{
    m_child1SizeFractionRequest = fraction;
    if (gtk_widget_get_mapped(gtkWidget()))
        setPositionInternally();
    else
        g_signal_connect_after(gtkWidget(),
                               "map",
                               G_CALLBACK(setPositionOnMapped),
                               this);
}

int Paned::sidePaneSize() const
{
    if (m_sidePaneSizeRequest > 0)
        return m_sidePaneSizeRequest;
    if (m_sidePaneIndex == 0)
        return gtk_paned_get_position(GTK_PANED(gtkWidget()));
    int totalSize;
    GValue handleSize = G_VALUE_INIT;
    if (gtk_orientable_get_orientation(GTK_ORIENTABLE(gtkWidget())) ==
        GTK_ORIENTATION_HORIZONTAL)
        totalSize = gtk_widget_get_allocated_width(gtkWidget());
    else
        totalSize = gtk_widget_get_allocated_height(gtkWidget());
    g_value_init(&handleSize, G_TYPE_INT);
    gtk_widget_style_get_property(gtkWidget(), "handle-size", &handleSize);
    return totalSize - g_value_get_int(&handleSize) -
        gtk_paned_get_position(GTK_PANED(gtkWidget()));
}

void Paned::setSidePaneSize(int size)
{
    m_sidePaneSizeRequest = size;
    if (gtk_widget_get_mapped(gtkWidget()))
        setPositionInternally();
    else
        g_signal_connect_after(gtkWidget(),
                               "map",
                               G_CALLBACK(setPositionOnMapped),
                               this);
}

}
