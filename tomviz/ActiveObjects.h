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
#ifndef tomvizActiveObjects_h
#define tomvizActiveObjects_h

#include <QObject>
#include <QPointer>
#include "DataSource.h"
#include "Module.h"

class pqView;
class vtkSMSessionProxyManager;
class vtkSMViewProxy;

namespace tomviz
{

/// ActiveObjects keeps track of active objects in TomViz.
/// This is similar to pqActiveObjects in ParaView, however tracks objects
/// relevant to TomViz.
class ActiveObjects : public QObject
  {
  Q_OBJECT

  typedef QObject Superclass;

public:
  /// Returns reference to the singleton instance.
  static ActiveObjects& instance();

  /// Returns the active view.
  vtkSMViewProxy* activeView() const;

  /// Returns the active data source.
  DataSource* activeDataSource() const
    { return this->ActiveDataSource; }

  /// Returns the active module.
  Module* activeModule() const
    { return this->ActiveModule; }

  /// Returns the vtkSMSessionProxyManager from the active server/session.
  /// Provided here for convenience, since we need to access the proxy manager
  /// often.
  vtkSMSessionProxyManager* proxyManager() const;

public slots:
  /// Set the active view;
  void setActiveView(vtkSMViewProxy*);

  /// Set the active data source.
  void setActiveDataSource(DataSource* source);

  /// Set the active module.
  void setActiveModule(Module* module);

  /// Renders all views.
  void renderAllViews();

signals:
  /// fired whenever the active view changes.
  void viewChanged(vtkSMViewProxy*);

  /// fired whenever the active data source changes.
  void dataSourceChanged(DataSource*);

  /// fired whenever the active module changes.
  void moduleChanged(Module*);

private slots:
  void viewChanged(pqView*);
  void dataSourceRemoved(DataSource*);
  void moduleRemoved(Module*);

protected:
  ActiveObjects();
  virtual ~ActiveObjects();

  QPointer<DataSource> ActiveDataSource;
  void* VoidActiveDataSource;

  QPointer<Module> ActiveModule;
  void* VoidActiveModule;

private:
  Q_DISABLE_COPY(ActiveObjects)
};
}
#endif
