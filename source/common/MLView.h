
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLActor.h"
#include "MLWidget.h"
#include "MLCollection.h"

namespace ml
{
  // View: a kind of Widget that holds other Widgets.
  //
	class View : public Widget
	{
	public:
		Collection< Widget > _widgets;

		// background widgets must never change in appearance, except when resized.
		// also they may be drawn in any order, so they should not visibly overlap.
		// each view has its own background, therefore a CollectionRoot.
		CollectionRoot< Widget > _backgroundWidgets;

		Widget* _stillDownWidget{ nullptr };
		Vec2 _dragStart{};

		// A view must be passed an existing Collection of Widgets on creation.
		View(Collection< Widget > t, WithValues v);

		// Widget interface
		void setDirty(bool d) override;
		MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) override;
		MessageList animate(int elapsedTimeInMs, DrawContext dc) override;
		void draw(DrawContext d) override;

		// View interface
		void drawWidget(const DrawContext& dc, Widget* w);
		void drawAllWidgets(DrawContext dc);
		void drawDirtyWidgets(DrawContext dc);

	private:
		void drawBackgroundWidget(const DrawContext& dc, Widget* w);

		Path _widgetPointerToName(Widget* w);
		std::vector< Widget* > findWidgetsForEvent(const GUIEvent& e);
		virtual void drawBackground(DrawContext dc, Rect nativeRect);
		size_t _frameCounter{ 0 };
		int framesSinceTick{ 0 };
		int testCounter{ 0 };
		int secsCounter{ 0 };
	};

}
