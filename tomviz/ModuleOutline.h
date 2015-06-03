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
#ifndef tomvizModuleOutline_h
#define tomvizModuleOutline_h

#include "Module.h"
#include "vtkWeakPointer.h"

class vtkSMSourceProxy;
class vtkSMProxy;

namespace tomviz
{

/// A simple module to show the outline for any dataset.
class ModuleOutline : public Module
{
  Q_OBJECT

  typedef Module Superclass;

public:
  ModuleOutline(QObject* parent=NULL);
  virtual ~ModuleOutline();

  virtual QString label() const { return  "Outline"; }
  virtual QIcon icon() const;
  virtual bool initialize(DataSource* dataSource, vtkSMViewProxy* view);
  virtual bool finalize();
  virtual bool setVisibility(bool val);
  virtual bool visibility() const;
  virtual void addToPanel(pqProxiesWidget*);
  virtual bool serialize(pugi::xml_node& ns) const;
  virtual bool deserialize(const pugi::xml_node& ns);

private:
  Q_DISABLE_COPY(ModuleOutline)
  vtkWeakPointer<vtkSMSourceProxy> OutlineFilter;
  vtkWeakPointer<vtkSMProxy> OutlineRepresentation;
};

}

#endif
