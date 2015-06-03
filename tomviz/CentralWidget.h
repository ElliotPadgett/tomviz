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
#ifndef tomvizCentralWidget_h
#define tomvizCentralWidget_h

#include <QScopedPointer>
#include <QWidget>
#include <QMap>
#include <QPointer>
#include <vtkNew.h>
#include <vtkWeakPointer.h>
#include <vtkSmartPointer.h>

class vtkObject;
class vtkSMSourceProxy;
class vtkContextView;
class vtkEventQtSlotConnect;
class vtkImageData;
class vtkTable;
class vtkScalarsToColors;

namespace tomviz
{
class DataSource;
class HistogramWorker;
class vtkChartHistogram;

/// CentralWidget is a QWidget that is used as the central widget
/// for the application. This include a histogram at the top and a
/// ParaView view-layout widget at the bottom.
class CentralWidget : public QWidget
{
  Q_OBJECT

  typedef QWidget Superclass;

public:
  CentralWidget(QWidget* parent = NULL, Qt::WindowFlags f = 0);
  virtual ~CentralWidget();

public slots:
  /// Set the data source to from which the data is "histogrammed" and shown
  /// in the histogram view.
  void setDataSource(DataSource*);

private slots:
  void histogramReady();
  void histogramClicked(vtkObject *caller);
  void refreshHistogram();

private:
  Q_DISABLE_COPY(CentralWidget)

  void setHistogramTable(vtkTable *table);

  class CWInternals;
  QScopedPointer<CWInternals> Internals;
  vtkNew<vtkContextView> Histogram;
  vtkNew<vtkChartHistogram> Chart;
  vtkNew<vtkEventQtSlotConnect> EventLink;
  QPointer<DataSource> ADataSource;
  HistogramWorker *Worker;
  QMap<vtkImageData *, vtkSmartPointer<vtkTable> > HistogramCache;
  vtkScalarsToColors *LUT;
};

}

#endif
