// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

#include "MLVGView.h"
#include "MLRenderer.h"
#include "MLWidget.h"
#include "MLView.h"

#include "pluginController.h"

constexpr int kGridUnitsX{ 16 };
constexpr int kGridUnitsY{ 4 };
constexpr int kDefaultGridSize{ 40 };
constexpr int kMinGridSize{ 25 };
constexpr int kMaxGridSize{ 125 };

const Rect kDefaultPopupSize{0, 0, 3.5, 3.5};
const Rect kDefaultPopupStartRect{0, 0, 1, 1};

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

//------------------------------------------------------------------------
class PluginEditorView :
public Steinberg::CPluginView,
public Steinberg::IPlugViewContentScaleSupport,
public ml::Renderer,
public View
{
public:
  /** Constructor. */
  PluginEditorView (PluginController& controller, int w, int h, void* platformHandle);
  
  /** Destructor. */
  virtual ~PluginEditorView ();
  
  // from ml::Renderer
  void initializeResources(NativeDrawContext* nvg) override;
  void viewResized(int, int) override;
  Vec2 getViewSize() override;
  void doRender(NativeDrawContext* nvg) override;
  void pushEvent(GUIEvent g) override;

  //bool PLUGIN_API open (void* parent) override;
  //void PLUGIN_API close () override;

  /** Sets the idle rate controlling the parameter update rate. */
  //void setIdleRate (int32 millisec);
  
  // IPlugViewContentScaleSupport interface
  virtual tresult PLUGIN_API setContentScaleFactor (ScaleFactor factor) SMTG_OVERRIDE;
  
  OBJ_METHODS (PluginEditorView, CPluginView)
  DEFINE_INTERFACES
  END_DEFINE_INTERFACES (CPluginView)
  REFCOUNT_METHODS (CPluginView)

  // TODO use platform handle in ctor
#if TARGET_OS_IPHONE
  static void setBundleRef (/*CFBundleRef*/ void* bundle);
#endif

  
  // Widget interface
  void processValueChange(ValueChange g) override;
//  ValueChangeList processInput(const GUICoordinates& gc, GUIEvent e) override;
//  void processSignal(DSPBuffer& sig) override {}
//  void animate(int elapsedTimeInMs) override;
//   void draw(ml::DrawContext d) override; // unused
  
  // View interface
//  std::vector< ml::Path > findWidgetsForEvent(const GUIEvent& e) const;
//  ValueChangeList sendEventToWidget(const GUICoordinates& gc, GUIEvent e, Path widgetPath);
//  void drawWidget(ml::DrawContext dc, Widget* w);
//  void drawAllWidgets(ml::DrawContext dc, Tree< std::unique_ptr< Widget > >& widgets);
  void drawBackground(DrawContext dc, bool drawGrid) override;

  
protected:
  
  //---from IPlugView-------
  tresult PLUGIN_API isPlatformTypeSupported (FIDString type) SMTG_OVERRIDE;
  tresult PLUGIN_API attached (void* parent, FIDString type) SMTG_OVERRIDE;
  tresult PLUGIN_API removed () SMTG_OVERRIDE;
  
  tresult PLUGIN_API onWheel (float distance) SMTG_OVERRIDE;
  tresult PLUGIN_API onKeyDown (char16 key, int16 keyMsg, int16 modifiers) SMTG_OVERRIDE;
  tresult PLUGIN_API onKeyUp (char16 key, int16 keyMsg, int16 modifiers) SMTG_OVERRIDE;
  
  /*
  tresult PLUGIN_API getSize (ViewRect* size) SMTG_OVERRIDE;
  tresult PLUGIN_API onSize (ViewRect* newSize) SMTG_OVERRIDE;
  */

  tresult PLUGIN_API onFocus (TBool state) SMTG_OVERRIDE { return Steinberg::kResultTrue; }
  
  // actually we can resize, but we do our own resizing. returning false
  // keeps hosts from using their own resize code.
  tresult PLUGIN_API canResize () SMTG_OVERRIDE { return Steinberg::kResultFalse; }
  
  tresult PLUGIN_API checkSizeConstraint (ViewRect* rect) SMTG_OVERRIDE { return Steinberg::kResultFalse; }

  Vec2 constrainSize(Vec2 size);

  ml::Path findWidgetForEvent(GUIEvent e) const;
  ml::Path findWidgetsForParameter(ml::Path paramName);

  void resizeEditor(Vec2 newSize);
  void drawResizer(DrawContext dc);
  
  void markEverythingAsDirty();
  
private:
  
  void cancelModalWidget();

  void updateClickAndHold(GUIEvent e);
  void dismissModalWidgetOnClick(GUIEvent e);
  void handleGUIEvents();
  void updateWidgets();

  int getElapsedTime();
  
  time_point< system_clock > _previousFrameTime;
  time_point< system_clock > _clickAndHoldStartTime;
  
  Vec2 _clickAndHoldStartPosition;

  PluginController& _controller;

  void* _platformHandle{ nullptr };
  
  std::unique_ptr<VGView> _mlvgView;
  
  int _animCounter{0};

  int _redrawAllUntilFrame{0};
  
  // Queue of input events from mouse, keyboard, etc.
  Queue< GUIEvent > _inputQueue{ 1024 };

  // images and tables used for drawing.
  Tree< std::unique_ptr< Resource > > _resources;
  
  Tree< std::vector< Widget* > > _widgetsByParameter;
  
  
  Tree< std::vector< Widget* > > _popupWidgetsByParameter;
  Tree< std::unique_ptr< ParameterDescription > > _popupParams;
  std::vector< Path > _popupParamNames;

  Rect _borderRect;
  
  NativeDrawContext* _drawContext{};
  
  ml::Timer _ioTimer;
  ml::Timer _clickAndHoldTimer;
  ml::Timer _animationTimer;
  ml::Path _activeModalWidget{};
  
  Vec2 _dragPrev;
  Vec2 resizerDragStartPosition{};
  Vec2 viewSizeAtDragStart{};
  
  void* _parent{};

  float _editorScale{ 1.0f };

  // for Steinberg display scale awareness API TODO
  float _contentScaleFactor{1.0f};
  
};

}}} // namespaces

