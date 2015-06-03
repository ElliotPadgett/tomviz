/******************************************************************************

  This source file is part of the tomviz project.

  Copyright Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/
#include "ModuleManager.h"

#include "DataSource.h"
#include "ModuleFactory.h"
#include "Module.h"
#include "Utilities.h"
#include "ActiveObjects.h"

#include "pqDeleteReaction.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkStdString.h"

#include <QPointer>
#include <QtDebug>
#include <QSet>
#include <QMap>

namespace tomviz
{

class ModuleManager::MMInternals
{
public:
  QList<QPointer<DataSource> > DataSources;
  QList<QPointer<Module> > Modules;
};

//-----------------------------------------------------------------------------
ModuleManager::ModuleManager(QObject* parentObject)
  : Superclass(parentObject),
  Internals(new ModuleManager::MMInternals())
{
}

//-----------------------------------------------------------------------------
ModuleManager::~ModuleManager()
{
  // Internals is a QScopedPointer.
}

//-----------------------------------------------------------------------------
ModuleManager& ModuleManager::instance()
{
  static ModuleManager theInstance;
  return theInstance;
}

//-----------------------------------------------------------------------------
void ModuleManager::reset()
{
  this->removeAllModules();
  this->removeAllDataSources();
  pqDeleteReaction::deleteAll();
}

//-----------------------------------------------------------------------------
void ModuleManager::addDataSource(DataSource* dataSource)
{
  if (dataSource && !this->Internals->DataSources.contains(dataSource))
    {
    dataSource->setParent(this);
    this->Internals->DataSources.push_back(dataSource);
    emit this->dataSourceAdded(dataSource);
    }
}

//-----------------------------------------------------------------------------
void ModuleManager::removeDataSource(DataSource* dataSource)
{
  if (this->Internals->DataSources.removeOne(dataSource))
    {
    emit this->dataSourceRemoved(dataSource);
    delete dataSource;
    }
}

//-----------------------------------------------------------------------------
void ModuleManager::removeAllDataSources()
{
  foreach (DataSource* dataSource, this->Internals->DataSources)
    {
    emit this->dataSourceRemoved(dataSource);
    delete dataSource;
    }
  this->Internals->DataSources.clear();
}

//-----------------------------------------------------------------------------
void ModuleManager::addModule(Module* module)
{
  if (!this->Internals->Modules.contains(module))
    {
    module->setParent(this);
    this->Internals->Modules.push_back(module);
    emit this->moduleAdded(module);
    }
}

//-----------------------------------------------------------------------------
void ModuleManager::removeModule(Module* module)
{
  if (this->Internals->Modules.removeOne(module))
    {
    emit this->moduleRemoved(module);
    delete module;
    }
}

//-----------------------------------------------------------------------------
void ModuleManager::removeAllModules()
{
  foreach (Module* module, this->Internals->Modules)
    {
    emit this->moduleRemoved(module);
    delete module;
    }
  this->Internals->Modules.clear();
}

//-----------------------------------------------------------------------------
void ModuleManager::removeAllModules(DataSource* source)
{
  Q_ASSERT(source);
  QList<Module*> modules;
  foreach (Module* module, this->Internals->Modules)
    {
    if (module->dataSource() == source)
      {
      modules.push_back(module);
      }
    }
  foreach (Module* module, modules)
    {
    this->removeModule(module);
    }
}

//-----------------------------------------------------------------------------
Module* ModuleManager::createAndAddModule(const QString& type,
                                          DataSource* dataSource,
                                          vtkSMViewProxy* view)
{
  if (!view || !dataSource)
    {
    return NULL;
    }

  // Create an outline module for the source in the active view.
  Module* module = ModuleFactory::createModule(type, dataSource, view);
  if (module)
    {
    this->addModule(module);
    }
  return module;
}

//-----------------------------------------------------------------------------
QList<Module*> ModuleManager::findModulesGeneric(DataSource* dataSource,
                                                 vtkSMViewProxy* view)
{
  QList<Module*> modules;
  foreach (Module* module, this->Internals->Modules)
    {
    if (module && module->dataSource() == dataSource &&
      (view == NULL || view == module->view()))
      {
      modules.push_back(module);
      }
    }
  return modules;
}

//-----------------------------------------------------------------------------
bool ModuleManager::serialize(pugi::xml_node& ns) const
{
  QSet<vtkSMSourceProxy*> uniqueOriginalSources;

  // Build a list of unique original data sources. These are the data readers.
  foreach (const QPointer<DataSource>& ds, this->Internals->DataSources)
    {
    if (ds == NULL || uniqueOriginalSources.contains(ds->originalDataSource()))
      {
      continue;
      }
    vtkSMSourceProxy* reader = ds->originalDataSource();
    Q_ASSERT(reader != NULL);
    pugi::xml_node odsnode = ns.append_child("OriginalDataSource");
    odsnode.append_attribute("id").set_value(reader->GetGlobalIDAsString());
    odsnode.append_attribute("xmlgroup").set_value(reader->GetXMLGroup());
    odsnode.append_attribute("xmlname").set_value(reader->GetXMLName());
    if (tomviz::serialize(reader, odsnode) == false)
      {
      qWarning() << "Failed to serialize data reader: " << reader->GetGlobalIDAsString();
      ns.remove_child(odsnode);
      continue;
      }
    uniqueOriginalSources.insert(reader);
    }

  // Now serialize each of the data-sources. The data sources don't serialize
  // the original data source since that can be shared among sources.
  QList<DataSource*> serializedDataSources;
  foreach (const QPointer<DataSource>& ds, this->Internals->DataSources)
    {
    if (ds && uniqueOriginalSources.contains(ds->originalDataSource()))
      {
      pugi::xml_node dsnode = ns.append_child("DataSource");
      dsnode.append_attribute("id").set_value(
        ds->producer()->GetGlobalIDAsString());
      dsnode.append_attribute("original_data_source").set_value(
        ds->originalDataSource()->GetGlobalIDAsString());
      if (ds == ActiveObjects::instance().activeDataSource())
        {
        dsnode.append_attribute("active").set_value(1);
        }
      if (!ds->serialize(dsnode))
        {
        qWarning("Failed to serialize DataSource.");
        ns.remove_child(dsnode);
        continue;
        }
      Q_ASSERT(serializedDataSources.contains(ds) == false);
      serializedDataSources.append(ds);
      }
    }

  // Now serialize each of the modules.
  foreach (const QPointer<Module>& mdl, this->Internals->Modules)
    {
    if (mdl && serializedDataSources.contains(mdl->dataSource()))
      {
      pugi::xml_node mdlnode = ns.append_child("Module");
      mdlnode.append_attribute("type").set_value(ModuleFactory::moduleType(mdl));
      mdlnode.append_attribute("data_source").set_value(
        mdl->dataSource()->producer()->GetGlobalIDAsString());
      mdlnode.append_attribute("view").set_value(
        mdl->view()->GetGlobalIDAsString());
      if (mdl == ActiveObjects::instance().activeModule())
        {
        mdlnode.append_attribute("active").set_value(1);
        }
      if (!mdl->serialize(mdlnode))
        {
        qWarning() << "Failed to serialize Module.";
        ns.remove_child(mdlnode);
        continue;
        }

      }
    }

  // Now serialize the views and layouts.
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(ActiveObjects::instance().proxyManager());
  iter->SetModeToOneGroup();
  for (iter->Begin("layouts"); !iter->IsAtEnd(); iter->Next())
    {
    if (vtkSMProxy* layout = iter->GetProxy())
      {
      pugi::xml_node lnode = ns.append_child("Layout");
      lnode.append_attribute("id").set_value(layout->GetGlobalIDAsString());
      lnode.append_attribute("xmlgroup").set_value(layout->GetXMLGroup());
      lnode.append_attribute("xmlname").set_value(layout->GetXMLName());
      if (!tomviz::serialize(layout, lnode))
        {
        qWarning("Failed to serialize layout.");
        ns.remove_child(lnode);
        }
      }
    }
  for (iter->Begin("views"); !iter->IsAtEnd(); iter->Next())
    {
    if (vtkSMProxy* view = iter->GetProxy())
      {
      pugi::xml_node vnode = ns.append_child("View");
      vnode.append_attribute("id").set_value(view->GetGlobalIDAsString());
      vnode.append_attribute("xmlgroup").set_value(view->GetXMLGroup());
      vnode.append_attribute("xmlname").set_value(view->GetXMLName());
      if (view == ActiveObjects::instance().activeView())
        {
        vnode.append_attribute("active").set_value(1);
        }
      if (!tomviz::serialize(view, vnode))
        {
        qWarning("Failed to serialize view.");
        ns.remove_child(vnode);
        }
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
bool ModuleManager::deserialize(const pugi::xml_node& ns)
{
  this->reset();

  vtkSMSessionProxyManager* pxm = ActiveObjects::instance().proxyManager();
  Q_ASSERT(pxm);

  // deserialize all views and layouts first.
  vtkNew<vtkSMProxyLocator> locator;
  for (pugi::xml_node node = ns.child("Layout"); node; node = node.next_sibling("Layout"))
    {
    vtkTypeUInt32 id = node.attribute("id").as_uint(0);
    const char* group = node.attribute("xmlgroup").value();
    const char* type = node.attribute("xmlname").value();
    if (group==NULL || type==NULL)
      {
      qWarning() << "Invalid xml for Layout with id " << id;
      continue;
      }
    vtkSmartPointer<vtkSMProxy> proxy;
    proxy.TakeReference(pxm->NewProxy(group, type));
    if (!tomviz::deserialize(proxy, node))
      {
      qWarning() << "Failed to create proxy of type: " << group << ", " << type;
      continue;
      }
    proxy->UpdateVTKObjects();
    pxm->RegisterProxy("layouts", proxy);
    locator->AssignProxy(id, proxy);
    }
  for (pugi::xml_node node = ns.child("View"); node; node = node.next_sibling("View"))
    {
    vtkTypeUInt32 id = node.attribute("id").as_uint(0);
    const char* group = node.attribute("xmlgroup").value();
    const char* type = node.attribute("xmlname").value();
    if (group==NULL || type==NULL)
      {
      qWarning() << "Invalid xml for View with id " << id;
      continue;
      }
    vtkSmartPointer<vtkSMProxy> proxy;
    proxy.TakeReference(pxm->NewProxy(group, type));
    if (!tomviz::deserialize(proxy, node, locator.GetPointer()))
      {
      qWarning() << "Failed to create proxy of type: " << group << ", " << type;
      continue;
      }
    proxy->UpdateVTKObjects();
    pxm->RegisterProxy("views", proxy);
    locator->AssignProxy(id, proxy);

    if (node.attribute("active").as_int(0) == 1)
      {
      ActiveObjects::instance().setActiveView(vtkSMViewProxy::SafeDownCast(proxy));
      }
    }

  // process all original data sources i.e. readers and create them.
  QMap<vtkTypeUInt32, vtkSmartPointer<vtkSMSourceProxy> > originalDataSources;
  for (pugi::xml_node odsnode = ns.child("OriginalDataSource"); odsnode;
    odsnode = odsnode.next_sibling("OriginalDataSource"))
    {
    vtkTypeUInt32 id = odsnode.attribute("id").as_uint(0);
    const char* group = odsnode.attribute("xmlgroup").value();
    const char* type = odsnode.attribute("xmlname").value();
    if (group==NULL || type==NULL)
      {
      qWarning() << "Invalid xml for OriginalDataSource with id " << id;
      continue;
      }

    vtkSmartPointer<vtkSMProxy> proxy;
    proxy.TakeReference(pxm->NewProxy(group, type));
    if (!tomviz::deserialize(proxy, odsnode))
      {
      qWarning() << "Failed to create proxy of type: " << group << ", " << type;
      continue;
      }
    proxy->UpdateVTKObjects();
    originalDataSources[id] = vtkSMSourceProxy::SafeDownCast(proxy);
    }

  // now, deserialize all data sources.
  QMap<vtkTypeUInt32, DataSource*> dataSources;
  for (pugi::xml_node dsnode = ns.child("DataSource"); dsnode;
    dsnode = dsnode.next_sibling("DataSource"))
    {
    vtkTypeUInt32 id = dsnode.attribute("id").as_uint(0);
    vtkTypeUInt32 odsid = dsnode.attribute("original_data_source").as_uint(0);
    if (id == 0 || odsid == 0)
      {
      qWarning() << "Invalid xml for DataSource with id " << id;
      continue;
      }
    if (!originalDataSources.contains(odsid))
      {
      qWarning()
        << "Skipping DataSource with id " << id
        << " since required OriginalDataSource is missing.";
      continue;
      }

    // create the data source.
    DataSource* dataSource = new DataSource(originalDataSources[odsid]);
    if (!dataSource->deserialize(dsnode))
      {
      qWarning()
        << "Failed to deserialze DataSource with id " << id
        << ". Skipping it";
      continue;
      }
    this->addDataSource(dataSource);
    dataSources[id] = dataSource;

    if (dsnode.attribute("active").as_int(0) == 1)
      {
      ActiveObjects::instance().setActiveDataSource(dataSource);
      }
    }

  // now, deserialize all the modules.
  for (pugi::xml_node mdlnode = ns.child("Module"); mdlnode;
    mdlnode = mdlnode.next_sibling("Module"))
    {
    const char* type = mdlnode.attribute("type").value();
    vtkTypeUInt32 dsid = mdlnode.attribute("data_source").as_uint(0);
    vtkTypeUInt32 viewid = mdlnode.attribute("view").as_uint(0);
    if (dataSources[dsid] == NULL ||
      vtkSMViewProxy::SafeDownCast(locator->LocateProxy(viewid)) == NULL)
      {
      qWarning() << "Failed to create module: " << type;
      continue;
      }

    // Create module.
    Module* module = ModuleFactory::createModule(type, dataSources[dsid],
      vtkSMViewProxy::SafeDownCast(locator->LocateProxy(viewid)));
    if (!module || !module->deserialize(mdlnode))
      {
      qWarning() << "Failed to create module: " << type;
      delete module;
      continue;
      }
    this->addModule(module);
    if (mdlnode.attribute("active").as_int(0) == 1)
      {
      ActiveObjects::instance().setActiveModule(module);
      }
    }
  return true;
}

} // end of namesapce tomviz
