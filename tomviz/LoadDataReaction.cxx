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
#include "LoadDataReaction.h"

#include "ActiveObjects.h"
#include "DataSource.h"
#include "ModuleManager.h"
#include "pqLoadDataReaction.h"
#include "pqPipelineSource.h"
#include "pqProxyWidgetDialog.h"
#include "RecentFilesMenu.h"
#include "Utilities.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkTrivialProducer.h"

#include <QFileDialog>

namespace tomviz
{
//-----------------------------------------------------------------------------
LoadDataReaction::LoadDataReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
LoadDataReaction::~LoadDataReaction()
{
}

//-----------------------------------------------------------------------------
void LoadDataReaction::onTriggered()
{
  loadData();
}

QList<DataSource*> LoadDataReaction::loadData()
{
  vtkNew<vtkSMParaViewPipelineController> controller;

  QStringList filters;
  filters << "Common file types (*.jpg *.jpeg *.png *.tiff *.tif *.raw *.dat *.bin *.txt)"
          << "JPeg Image files (*.jpg *.jpeg)"
          << "PNG Image files (*.png)"
          << "TIFF Image files (*.tiff *.tif)"
          << "Raw data files (*.raw *.dat *.bin)"
          << "Text files (*.txt)"
          << "All files (*.*)";

  QFileDialog dialog(NULL);
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setNameFilters(filters);
  dialog.setObjectName("FileOpenDialog-tomviz"); // avoid name collision?

  QList<DataSource*> dataSources;
  if (dialog.exec())
  {
    QStringList filenames = dialog.selectedFiles();
    foreach(QString file, filenames)
    {
      dataSources << loadData(file);
    }
  }

  return dataSources;
}

DataSource* LoadDataReaction::loadData(const QString &fileName)
{
  vtkNew<vtkSMParaViewPipelineController> controller;
  QStringList files;
  files << fileName;
  pqPipelineSource* reader = pqLoadDataReaction::loadData(files);

  DataSource* dataSource = createDataSource(reader->getProxy());
  // dataSource may be NULL if user cancelled the action.
  if (dataSource)
    {
    // add the file to recent files menu.
    RecentFilesMenu::pushDataReader(reader->getProxy());
    }
  controller->UnRegisterProxy(reader->getProxy());
  return dataSource;
}

//-----------------------------------------------------------------------------
DataSource* LoadDataReaction::createDataSource(vtkSMProxy* reader)
{
  // Prompt user for reader configuration.
  pqProxyWidgetDialog dialog(reader);
  dialog.setObjectName("ConfigureReaderDialog");
  dialog.setWindowTitle("Configure Reader Parameters");
  if (dialog.hasVisibleWidgets() == false || dialog.exec() == QDialog::Accepted)
    {
    DataSource* dataSource = new DataSource(vtkSMSourceProxy::SafeDownCast(reader));
    // do whatever we need to do with a new data source.
    LoadDataReaction::dataSourceAdded(dataSource);
    return dataSource;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void LoadDataReaction::dataSourceAdded(DataSource* dataSource)
{
  ModuleManager::instance().addDataSource(dataSource);

  vtkSMViewProxy* view = ActiveObjects::instance().activeView();
  // Create an outline module for the source in the active view.
  if (Module* module = ModuleManager::instance().createAndAddModule(
      "Outline", dataSource, view))
    {
    ActiveObjects::instance().setActiveModule(module);
    }
  if (Module* module = ModuleManager::instance().createAndAddModule(
        "Orthogonal Slice", dataSource, view))
    {
    ActiveObjects::instance().setActiveModule(module);
    }
}

} // end of namespace tomviz
