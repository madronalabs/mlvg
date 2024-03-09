
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
#include "MLMessage.h"
#include "MLParameters.h"

namespace ml {

    // A Widget is a drawable UI element stored in a View.
    // its appearance in the view depends on two kinds of values:
    // Parameters and Properties.
    //
    // Parameters are quantities in the app or plugin Processor that
    // the Widget may control.
    //
    // Properties are variables that change the appearance of
    // the Widget. To store them, Widget inherits from PropertyTree.
    //
    class Widget : public PropertyTree, public MessageReceiver
    {
    public:

        Widget(WithValues p) : PropertyTree(p) {}
        Widget() = default;
        virtual ~Widget() = default;

        // engaged should be true when the Widget is currently responding to an ongoing gesture,
        // as a dial does when dragging. Single clicks will not set this flag.
        bool engaged{ false };

        // true if the Widget needs to be redrawn.
        bool _dirty{ true };

        // internal flag for View
        bool _needsDraw{ false };

    protected:

        // This is where the values, projections and descriptions of any
        // program parameters we control are stored.
        ParameterTree _params;

        // set the value of the named parameter and mark the Widget dirty.
        // this is not virtual. To override its behavior, instead intercept the
        // set_param message in handleMessage in your Widget and do something
        // else there.
        void setParamValue(Path paramName, Value v)
        {
            _params.setFromNormalizedValue(paramName, v);
            _dirty = true;
        }

        void setRealParamValue(Path paramName, Value v)
        {
            _params.setValue(paramName, v);
            _dirty = true;
        }

    public:

        // set our dirty flag. Views need to override this to also set
        // the flags of widgets they contain.
        virtual void setDirty(bool d) { _dirty = d; }
        bool isDirty() { return _dirty; }

        // default implementation of Widget::handleMessage:
        // set a param value or an internal property and mark self as dirty.
        void handleMessage(Message msg, MessageList* /* replyPtr */) override
        {
            switch (hash(head(msg.address)))
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
        // (and to be in the same address space!)
        // Most Widgets shouldn't need this.
        virtual void receiveNamedRawPointer(Path name, void* ptr) {}

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
        // Widgets can also override this to do any setup based on param
        // descriptions. This base class method should also be called afterwards!
        //
        virtual void setupParams()
        {
            if (hasProperty("param"))
            {
                // get the parameter name for a single-parameter Widget.
                // Widgets with multiple parameters must override setupParams().
                Path paramName(getTextProperty("param"));

                // check that the description from the parameter tree exists.
                // if not, make a generic description.
                if (!_params.descriptions[paramName])
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
            return _params.getNormalizedValue(paramName);
        }

        inline Value getRealParamValue(Path paramName)
        {
            return _params.getRealValue(paramName);
        }

        // process user input, modify our internal state, and generate ValueChanges.
        // input coordinates are in the parent view's grid coordinate system.
        virtual MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) { return MessageList{}; }

        // process data from a published Signal, for signal viewers. Most Widgets don't implement this.
        virtual void processPublishedSignal(Value sigVal) {}

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
        inline ml::Rect getRectProperty(Path p) const { return matrixToRect(getMatrixProperty(p)); }
        inline ml::Rect getRectPropertyWithDefault(Path p, ml::Rect r) const { return matrixToRect(getMatrixPropertyWithDefault(p, rectToMatrix(r))); }
        inline void setRectProperty(Path p, ml::Rect r) { setProperty(p, rectToMatrix(r)); }

        inline ml::Rect getBounds() const { return getRectProperty("bounds"); }
        inline void setBounds(ml::Rect r) { setProperty("bounds", rectToMatrix(r)); }

        inline ml::Vec2 getPointProperty(Path p) const { return matrixToVec2(getMatrixProperty(p)); }
        inline ml::Vec2 getPointPropertyWithDefault(Path p, ml::Vec2 r) const { return matrixToVec2(getMatrixPropertyWithDefault(p, vec2ToMatrix(r))); }
        inline void setPointProperty(Path p, ml::Vec2 r) { setProperty(p, vec2ToMatrix(r)); }

        inline NVGcolor getColorProperty(Path p) const { return matrixToColor(getMatrixProperty(p)); }
        inline NVGcolor getColorPropertyWithDefault(Path p, NVGcolor r) const { return matrixToColor(getMatrixPropertyWithDefault(p, colorToMatrix(r))); }
        inline void setColorProperty(Path p, NVGcolor r) { setProperty(p, colorToMatrix(r)); }
    };

    // utilities

    // get bounds in the native coordinates that will be current when the widget
    // is drawn, with the origin at the top left of its bounding box.
    inline ml::Rect getLocalBounds(const DrawContext& dc, const Widget& w)
    {
        return roundToInt(alignTopLeftToOrigin(dc.coords.gridToPixel(w.getBounds())));
    }

    inline ml::Rect getPixelBounds(const DrawContext& dc, const Widget& w)
    {
        return roundToInt(dc.coords.gridToPixel(w.getBounds()));
    }

    inline ml::Rect getCurrentAndPreviousBounds(const Widget& w)
    {
        // not needed? w.hasProperty("bounds")
        ml::Rect currentBounds = w.getBounds();
        if (w.getProperty("previous_bounds"))
        {
            ml::Rect previousBounds = matrixToRect(w.getProperty("previous_bounds").getMatrixValue());
            currentBounds = rectEnclosing(currentBounds, previousBounds);
        }
        return currentBounds;
    }




} // namespace ml
