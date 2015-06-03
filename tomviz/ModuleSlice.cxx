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
#include "ModuleSlice.h"

#include "DataSource.h"
#include "pqProxiesWidget.h"
#include "Utilities.h"

#include "vtkAlgorithm.h"
#include "vtkNonOrthoImagePlaneWidget.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarsToColors.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSMViewProxy.h"

namespace tomviz
{

//-----------------------------------------------------------------------------
ModuleSlice::ModuleSlice(QObject* parentObject) : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
ModuleSlice::~ModuleSlice()
{
  this->finalize();
}

//-----------------------------------------------------------------------------
QIcon ModuleSlice::icon() const
{
  return QIcon(":/pqWidgets/Icons/pqSlice24.png");
}

//-----------------------------------------------------------------------------
bool ModuleSlice::initialize(DataSource* dataSource, vtkSMViewProxy* view)
{
  if (!this->Superclass::initialize(dataSource, view))
    {
    return false;
    }

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  vtkSMSourceProxy* producer = dataSource->producer();
  vtkSMSessionProxyManager* pxm = producer->GetSessionProxyManager();

  // Create the pass through filter.
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("filters", "PassThrough"));

  this->PassThrough = vtkSMSourceProxy::SafeDownCast(proxy);
  Q_ASSERT(this->PassThrough);
  controller->PreInitializeProxy(this->PassThrough);
  vtkSMPropertyHelper(this->PassThrough, "Input").Set(producer);
  controller->PostInitializeProxy(this->PassThrough);
  controller->RegisterPipelineProxy(this->PassThrough);

  //Create the widget
  const bool widgetSetup = this->setupWidget(view,producer);

  if(widgetSetup)
    {
    this->Widget->On();
    this->Widget->InteractionOn();
    }

  Q_ASSERT(this->Widget);
  return widgetSetup;
}


//-----------------------------------------------------------------------------
//should only be called from initialize after the PassThrough has been setup
bool ModuleSlice::setupWidget(vtkSMViewProxy* view, vtkSMSourceProxy* producer)
{
  vtkSMSessionProxyManager* pxm = producer->GetSessionProxyManager();
  vtkAlgorithm* passThroughAlg = vtkAlgorithm::SafeDownCast(
                                 this->PassThrough->GetClientSideObject());

  vtkRenderWindowInteractor* rwi = view->GetRenderWindow()->GetInteractor();

  //determine the name of the property we are coloring by
  const char* propertyName = producer->GetDataInformation()->
                                       GetPointDataInformation()->
                                       GetArrayInformation(0)->
                                       GetName();

  if(!rwi||!passThroughAlg||!propertyName)
    {
    return false;
    }

  this->Widget = vtkSmartPointer<vtkNonOrthoImagePlaneWidget>::New();

  //set the interactor on the widget to be what the current
  //render window is using
  this->Widget->SetInteractor( rwi );

  //setup the color of the border of the widget
  {
  double color[3] = {1, 0, 0};
  this->Widget->GetPlaneProperty()->SetColor(color);
  }

  //turn texture interpolation to be linear
  this->Widget->TextureInterpolateOn();
  this->Widget->SetResliceInterpolateToLinear();

  //Construct the transfer function proxy for the widget
  vtkSMProxy* lut = this->colorMap();

  //set the widgets lookup table to be the one that the transfer function
  //manager is using
  vtkScalarsToColors* stc =
    vtkScalarsToColors::SafeDownCast(lut->GetClientSideObject());
  this->Widget->SetLookupTable(stc);

  //lastly we set up the input connection
  this->Widget->SetInputConnection(passThroughAlg->GetOutputPort());

  Q_ASSERT(rwi);
  Q_ASSERT(passThroughAlg);
  return true;
}

//-----------------------------------------------------------------------------
void ModuleSlice::updateColorMap()
{
  Q_ASSERT(this->Widget);

  //Construct the transfer function proxy for the widget
  vtkSMProxy* lut = this->colorMap();

  //set the widgets lookup table to be the one that the transfer function
  //manager is using
  vtkScalarsToColors* stc =
    vtkScalarsToColors::SafeDownCast(lut->GetClientSideObject());
  this->Widget->SetLookupTable(stc);
}

//-----------------------------------------------------------------------------
bool ModuleSlice::finalize()
{
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  controller->UnRegisterProxy(this->PassThrough);

  this->PassThrough = NULL;

  if(this->Widget != NULL)
    {
    this->Widget->InteractionOff();
    this->Widget->Off();
    }

  return true;
}

//-----------------------------------------------------------------------------
bool ModuleSlice::setVisibility(bool val)
{
  Q_ASSERT(this->Widget);
  this->Widget->SetEnabled(val ? 1 : 0);
  return true;
}

//-----------------------------------------------------------------------------
bool ModuleSlice::visibility() const
{
  Q_ASSERT(this->Widget);
  return this->Widget->GetEnabled() != 0;
}

//-----------------------------------------------------------------------------
bool ModuleSlice::serialize(pugi::xml_node& ns) const
{
  // FIXME: serialize slice properties.
  return this->Superclass::serialize(ns);
}

//-----------------------------------------------------------------------------
bool ModuleSlice::deserialize(const pugi::xml_node& ns)
{
  // FIXME: deserialize slice properties.
  return this->Superclass::deserialize(ns);
}

}
