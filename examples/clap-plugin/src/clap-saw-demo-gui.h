#pragma once

#include "CLAPExport.h"
#include "nanovg.h"
#include <string>

constexpr int kGridUnitsX{ 10 };
constexpr int kGridUnitsY{ 5 };
constexpr int kDefaultGridSize{ 60 };
constexpr int kMinGridSize{ 30 };
constexpr int kMaxGridSize{ 120 };

// Forward declaration
class ClapSawDemo;

// Minimal GUI class - only implement what's specific to your plugin
class ClapSawDemoGUI : public ml::CLAPAppView<ClapSawDemo> {
public:
  // Constructor
  ClapSawDemoGUI(ClapSawDemo* processor);
  ~ClapSawDemoGUI() override = default;

  // Create your specific widgets
  void makeWidgets() override;

  // Layout widgets with consistent positioning
  void layoutView(ml::DrawContext dc) override;

  // Set up your visual style
  void initializeResources(NativeDrawContext* nvg) override;

private:

  // Helper function to load fonts from disk
  // void loadFontFromFile(NativeDrawContext* nvg, const std::string& fontName, const std::string& filePath);

};
