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
#ifndef tomvizViewPropertiesPanel_h
#define tomvizViewPropertiesPanel_h

#include <QWidget>
#include <QScopedPointer>

class vtkSMViewProxy;

namespace tomviz
{
class ViewPropertiesPanel : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  ViewPropertiesPanel(QWidget* parent=0);
  virtual ~ViewPropertiesPanel();

private slots:
  void setView(vtkSMViewProxy*);
  void render();
  void updatePanel();

private:
  Q_DISABLE_COPY(ViewPropertiesPanel)

  class VPPInternals;
  const QScopedPointer<VPPInternals> Internals;
};

}

#endif
