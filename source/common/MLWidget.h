
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "MLDrawContext.h"
#include "MLGUICoordinates.h"
#include "MLGUIEvent.h"
#include "MLValue.h"
#include "MLValueChange.h"
#include "MLTree.h"
#include "MLDSPOps.h"
#include "MLDSPBuffer.h"
#include "MLPropertyTree.h"
// 
#include "MLMessage.h"
#include "MLParameters.h"

namespace ml{

// A Widget is a drawable UI element stored in a View.
// It may control one or more parameters in the app or plugin.
// Any other variables that change the appearance of a Widget are
// stored in its PropertyTree.
//
class Widget : public PropertyTree, public MessageReceiver
{
public:
  
  Widget(WithValues p) : PropertyTree(p) {}
  Widget() = default;
  virtual ~Widget() = default;
  
  // engaged should be true when the Widget is currently responding to an ongoing gesture,
  // as a dial does when dragging. Single clicks will not set this flag.
  bool engaged{false};
  
protected:
  
  // This is where the values, projections and descriptions of any
  // program parameters we control are stored.
  ParameterTreeNormalized _params;
  
  // true if the Widget needs to be redrawn.
  bool _dirty{true};
    
  // set the value of the named parameter and mark the Widget dirty.
  // this is not virtual. To override its behavior, instead intercept the
  // set_param message in handleMessage in your Widget and do something
  // else there.
  void setParamValue(Path paramName, Value v)
  {
    _params[paramName] = v;
    _dirty = true;
  }

public:

  // set our dirty flag. Views need to override this to also set
  // the flags of widgets they contain.
  virtual void setDirty(bool d) { _dirty = d; }
  bool isDirty() { return _dirty; }

  // default implementation of Widget::handleMessage:
  // set a param value or an internal property and mark self as dirty.
  virtual void handleMessage(Message msg, Message*)
  {
    switch(hash(head(msg.address)))
    {
      case(hash("set_param")):
      {
        setParamValue(tail(msg.address), msg.value);
        _dirty = true;
        break;
      }
      case(hash("set_prop")):
      {
        setProperty(tail(msg.address), msg.value);
        _dirty = true;
        break;
      }
      case(hash("do")):
      {
        // default widget: nothing to do
        break;
      }
      default:
      {
        // default widget: no forwarding
        break;
      }
    }
  }
  
  // We might like to send a Widget a pointer to some resource it
  // can use, if that resource is guaranteed to outlive the Widget.
  // (and to be on the same computer!)
  // Most Widgets shouldn't need this.
  virtual void receiveNamedRawPointer(Path name, void* ptr){}
 
  // Most Widgets will have one parameter or none.
  // if a Widget has more parameters, it must overload this method to
  // return true for any parameter names it wants access to.
  //
  virtual bool knowsParam(Path paramName)
  {
    return Path(getTextProperty("param")) == paramName;
  }
  
  // in order to avoid lots of defensive checking later, this method
  // should be called after parameter setup and before any drawing
  // is done to make sure all parameters we are interested in have
  // projections and descriptions.
  //
  // Widgets can also overload this to do any setup based on param
  // descriptions. This base class method should also be called afterwards!
  //
  virtual void setupParams()
  {
    if(hasProperty("param"))
    {
      // get the parameter name for a single-parameter Widget.
      // Widgets with multiple parameters must override setupParams().
      Path paramName (getTextProperty("param"));
      
      // check that the description from the parameter tree exists.
      // if not, make a generic description.
      if(!_params.descriptions[paramName])
      {
        ParameterDescription defaultDesc;
        // defaultDesc.setProperty("name", pathToText(paramName));
        setParameterInfo(_params, paramName, defaultDesc);
        
        // warning not always germane: wild card params and Controller local properties won't be found
        // std::cout << "warning: no parameter " << paramName << " for a Widget that wants it\n";
      }
    }
  }
  
  inline void setParameterDescription(Path paramName, const ParameterDescription& paramDesc)
  {
    setParameterInfo(_params, paramName, paramDesc);
  }
  
  inline void makeProjectionForParameter(Path paramName)
  {
    _params.projections[paramName] = createParameterProjection(*_params.descriptions[paramName]);
  }
  
  inline Value getParamValue(Path paramName)
  {
    return _params[paramName];
  }

  // process user input, modify our internal state, and generate ValueChanges.
  // input coordinates are in the parent view's grid coordinate system.
  virtual MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) { return MessageList{}; }
  
  // process one DSPVector of signal input, for signal viewers. Most Widgets don't implement this.
  virtual void processSignal(DSPVector sig, size_t channel = 0) {}
  
  // called by the editor each frame just before drawing. This allows Widgets to
  // update any properties based on the time elapsed since the last frame.
  // This is separate from draw() because each Widget needs to calculate its new
  // bounds rect before the new frame is drawn.
  //
  // Note that invisible Widgets will also be animated. This allows a Widget
  // to show or hide itself in its animate() method. 
  virtual MessageList animate(int elapsedTimeInMs, DrawContext d) { return MessageList{}; }
  
  // give Widgets a chance to do things like make internal buffers on resize.
  virtual void resize(DrawContext d) {}

  // draw into the context d. The context's coordinates and drawing engine
  // will be set up for this Widget with the origin on the top left of its bounding box.
  // the context will be restored to its current state after the call.
  virtual void draw(DrawContext d) {}
    
  // property helpers
  ml::Rect getRectProperty(Path p) const { return matrixToRect(getMatrixProperty(p)); }
  ml::Rect getRectPropertyWithDefault(Path p, ml::Rect r) const { return matrixToRect(getMatrixPropertyWithDefault(p, rectToMatrix(r))); }
  // should be overloaded setProperty?
  void setRectProperty(Path p, ml::Rect r) { setProperty(p, rectToMatrix(r)); }

  ml::Vec2 getPointProperty(Path p) const { return matrixToVec2(getMatrixProperty(p)); }
  ml::Vec2 getPointPropertyWithDefault(Path p, ml::Vec2 r) const { return matrixToVec2(getMatrixPropertyWithDefault(p, vec2ToMatrix(r))); }
  void setPointProperty(Path p, ml::Vec2 r) { setProperty(p, vec2ToMatrix(r)); }

  NVGcolor getColorProperty(Path p) const { return matrixToColor(getMatrixProperty(p)); }
  NVGcolor getColorPropertyWithDefault(Path p, NVGcolor r) const { return matrixToColor(getMatrixPropertyWithDefault(p, colorToMatrix(r))); }
  void setColorProperty(Path p, NVGcolor r) { setProperty(p, colorToMatrix(r)); }
};

// utilities

// get bounds within parent in Rect format
inline ml::Rect getBounds(const Widget& w)
{
  return matrixToRect(w.getProperty("bounds").getMatrixValueWithDefault({0, 0, 0, 0}));
}

inline void setBoundsRect(Widget& w, ml::Rect b)
{
  w.setProperty("bounds", rectToMatrix(b));
}

// get bounds in the native coordinates that will be current when the widget
// is drawn, with the origin at the top left of its bounding box.
inline ml::Rect getLocalBounds(const ml::DrawContext& dc, const Widget& w)
{
  return roundToInt(alignTopLeftToOrigin(dc.coords.gridToNative(getBounds(w))));
}

inline ml::Rect getPixelBounds(const ml::DrawContext& dc, const Widget& w)
{
  return roundToInt(dc.coords.gridToNative(getBounds(w)));// + ml::Rect{0.5, 0.5, 0., 0.};
}

} // namespace ml
