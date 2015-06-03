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
#include <QApplication>

#include "pqOptions.h"
#include "pqPVApplicationCore.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include "MainWindow.h"
#include "TomVizPythonConfig.h"

#include <clocale>

// We need to override the default options to enable streaming by default.
// Streaming needs to be enabled for the dax representations
class TomoOptions : public pqOptions
{
public:
  static TomoOptions* New();
  vtkTypeMacro(TomoOptions, pqOptions);
  virtual int GetEnableStreaming() { return 1; }

protected:
  TomoOptions(): pqOptions()
  { }
};
vtkStandardNewMacro(TomoOptions);

int main(int argc, char** argv)
{
  QCoreApplication::setApplicationName("TomViz");
  QCoreApplication::setApplicationVersion("0.1.0");
  QCoreApplication::setOrganizationName("Kitware");

  tomviz::InitializePythonEnvironment(argc, argv);

  QApplication app(argc, argv);
  setlocale(LC_NUMERIC, "C");
  vtkNew<TomoOptions> options;
  pqPVApplicationCore appCore(argc, argv, options.GetPointer());
  tomviz::MainWindow window;
  window.show();
  return app.exec();
}
