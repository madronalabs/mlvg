#include "clap-saw-demo-gui.h"
#include "clap-saw-demo.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Include embedded font resources
#include "../build/resources/clap-saw-demo/resources.c"

ClapSawDemoGUI::ClapSawDemoGUI(ClapSawDemo* processor)
  : CLAPAppView("ClapSawDemo", processor) {

  // Set up grid system for fixed aspect ratio (following MLVG pattern)
  setGridSizeDefault(kDefaultGridSize);
  setGridSizeLimits(kMinGridSize, kMaxGridSize);
  setFixedAspectRatio({kGridUnitsX, kGridUnitsY});

}

void ClapSawDemoGUI::makeWidgets() {

  _view->_backgroundWidgets.add_unique<TextLabelBasic>("title", ml::WithValues{
    {"bounds", {2, 0.2, 6, 0.6}},
    {"text", "madronalib/mlvg demo"},
    {"font", "d_din"},
    {"text_size", _drawingProperties.getFloatProperty("title_text_size")},
    {"h_align", "center"},
    {"v_align", "middle"},
    {"text_color", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 })}
  });

  // Could be fun to have some kind of patch diagram in the middle of the GUI
  //
  //   ml::kPitch  ml::kGate
  //      |           |
  //   ml::SawGen     |
  //      |           |
  //   ml::Lopass  ml::ADSR
  //      |___________|
  //            *
  //            |

  // Cutoff
  _view->_widgets.add_unique<DialBasic>("f0", ml::WithValues{
    {"bounds", {_drawingProperties.getFloatProperty("left_col_x"),
                _drawingProperties.getFloatProperty("top_row_y"),
                _drawingProperties.getFloatProperty("large_dial_width"),
                _drawingProperties.getFloatProperty("large_dial_height")}},
    {"log", true},
    {"visible", true},
    {"draw_number", true},
    {"text_size", _drawingProperties.getFloatProperty("dial_text_size")},
    {"param", "f0"}
  });

  _view->_backgroundWidgets.add_unique<TextLabelBasic>("f0_label", ml::WithValues{
    {"text", "Cutoff"},
    {"font", "d_din"},
    {"text_size", _drawingProperties.getFloatProperty("label_text_size")},
    {"h_align", "right"},
    {"v_align", "middle"},
    {"text_color", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 })}
  });

  // Resonance
  _view->_widgets.add_unique<DialBasic>("Q", ml::WithValues{
    {"bounds", {_drawingProperties.getFloatProperty("right_col_x"),
                _drawingProperties.getFloatProperty("top_row_y"),
                _drawingProperties.getFloatProperty("large_dial_width"),
                _drawingProperties.getFloatProperty("large_dial_height")}},
    {"visible", true},
    {"draw_number", true},
    {"text_size", _drawingProperties.getFloatProperty("dial_text_size")},
    {"param", "Q"}
  });

  _view->_backgroundWidgets.add_unique<TextLabelBasic>("Q_label", ml::WithValues{
    {"text", "Q"},
    {"font", "d_din"},
    {"text_size", _drawingProperties.getFloatProperty("label_text_size")},
    {"h_align", "right"},
    {"v_align", "middle"},
    {"text_color", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 })}
  });

  // Attack
  _view->_widgets.add_unique<DialBasic>("attack", ml::WithValues{
    {"bounds", {_drawingProperties.getFloatProperty("adsr_col1_x"),
                _drawingProperties.getFloatProperty("bottom_row_y"),
                _drawingProperties.getFloatProperty("small_dial_width"),
                _drawingProperties.getFloatProperty("small_dial_height")}},
    {"visible", true},
    {"draw_number", true},
    {"text_size", _drawingProperties.getFloatProperty("dial_text_size")},
    {"param", "attack"}
  });

  _view->_backgroundWidgets.add_unique<TextLabelBasic>("attack_label", ml::WithValues{
    {"text", "Attack"},
    {"font", "d_din"},
    {"text_size", _drawingProperties.getFloatProperty("label_text_size")},
    {"h_align", "right"},
    {"v_align", "middle"},
    {"text_color", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 })}
  });

  // Decay
  _view->_widgets.add_unique<DialBasic>("decay", ml::WithValues{
    {"bounds", {_drawingProperties.getFloatProperty("adsr_col2_x"),
                _drawingProperties.getFloatProperty("bottom_row_y"),
                _drawingProperties.getFloatProperty("small_dial_width"),
                _drawingProperties.getFloatProperty("small_dial_height")}},
    {"visible", true},
    {"draw_number", true},
    {"text_size", _drawingProperties.getFloatProperty("dial_text_size")},
    {"param", "decay"}
  });

  _view->_backgroundWidgets.add_unique<TextLabelBasic>("decay_label", ml::WithValues{
    {"text", "Decay"},
    {"font", "d_din"},
    {"text_size", _drawingProperties.getFloatProperty("label_text_size")},
    {"h_align", "right"},
    {"v_align", "middle"},
    {"text_color", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 })}
  });

  // Sustain
  _view->_widgets.add_unique<DialBasic>("sustain", ml::WithValues{
    {"bounds", {_drawingProperties.getFloatProperty("adsr_col3_x"),
                _drawingProperties.getFloatProperty("bottom_row_y"),
                _drawingProperties.getFloatProperty("small_dial_width"),
                _drawingProperties.getFloatProperty("small_dial_height")}},
    {"visible", true},
    {"draw_number", true},
    {"text_size", _drawingProperties.getFloatProperty("dial_text_size")},
    {"param", "sustain"}
  });

  _view->_backgroundWidgets.add_unique<TextLabelBasic>("sustain_label", ml::WithValues{
    {"text", "Sustain"},
    {"font", "d_din"},
    {"text_size", _drawingProperties.getFloatProperty("label_text_size")},
    {"h_align", "right"},
    {"v_align", "middle"},
    {"text_color", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 })}
  });

  // Release
  _view->_widgets.add_unique<DialBasic>("release", ml::WithValues{
    {"bounds", {_drawingProperties.getFloatProperty("adsr_col4_x"),
                _drawingProperties.getFloatProperty("bottom_row_y"),
                _drawingProperties.getFloatProperty("small_dial_width"),
                _drawingProperties.getFloatProperty("small_dial_height")}},
    {"visible", true},
    {"draw_number", true},
    {"text_size", _drawingProperties.getFloatProperty("dial_text_size")},
    {"param", "release"}
  });

  _view->_backgroundWidgets.add_unique<TextLabelBasic>("release_label", ml::WithValues{
    {"text", "Release"},
    {"font", "d_din"},
    {"text_size", _drawingProperties.getFloatProperty("label_text_size")},
    {"h_align", "right"},
    {"v_align", "middle"},
    {"text_color", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 })}
  });

  // Add resize widget to bottom right corner
  _view->_widgets.add_unique<Resizer>("resizer", ml::WithValues{
    {"fix_ratio", static_cast<float>(kGridUnitsX)/static_cast<float>(kGridUnitsY)},  // Use grid constants for aspect ratio
    {"z", -2},                  // Stay on top of other widgets
    {"fixed_size", true},
    {"fixed_bounds", {-16, -16, 16, 16}},  // 16x16 pixel resize handle
    {"anchor", {1, 1}}          // Anchor to bottom right corner {1, 1}
  });
}

void ClapSawDemoGUI::layoutView(ml::DrawContext dc) {

  // Helper function to position labels under dials consistently
  auto positionLabelUnderDial = [&](ml::Path dialName, ml::Path labelName) {
    // Safety check: ensure both widgets exist before accessing them
    if (!_view->_widgets[dialName] || !_view->_backgroundWidgets[labelName]) {
      return;
    }
    ml::Rect dialRect = _view->_widgets[dialName]->getRectProperty("bounds");
    ml::Rect labelRect(0, 0, 3, 0.4); // TODO: make this smarter, using label's bounds?
    _view->_backgroundWidgets[labelName]->setRectProperty(
      "bounds",
      ml::alignCenterToPoint(labelRect, dialRect.bottomCenter() - ml::Vec2(0.4, 0.50))
    );
  };

  // Position lopass dials
  positionLabelUnderDial("f0", "f0_label");
  positionLabelUnderDial("Q", "Q_label");

  // Position ADSR labels
  positionLabelUnderDial("attack", "attack_label");
  positionLabelUnderDial("decay", "decay_label");
  positionLabelUnderDial("sustain", "sustain_label");
  positionLabelUnderDial("release", "release_label");
}

// void ClapSawDemoGUI::loadFontFromFile(NativeDrawContext* nvg, const std::string& fontName, const std::string& filePath) {
//   FILE* fontFile = fopen(filePath.c_str(), "rb");
//   if (fontFile) {
//     // Read font file into memory
//     fseek(fontFile, 0, SEEK_END);
//     long fontSize = ftell(fontFile);
//     fseek(fontFile, 0, SEEK_SET);
//     unsigned char* fontData = (unsigned char*)malloc(fontSize);
//     if (fontData) {
//       size_t bytesRead = fread(fontData, 1, fontSize, fontFile);
//       if (bytesRead == fontSize) {
//         // Create FontResource and store in resources map
//         _resources.fonts[ml::Path(fontName.c_str())] = std::make_unique<ml::FontResource>(
//           nvg, fontName.c_str(), fontData, fontSize, 1  // freeData=1 so FontResource owns the data
//         );
//       } else {
//         free(fontData);
//       }
//     }
//     fclose(fontFile);
//   } else {
//   }
// }

void ClapSawDemoGUI::initializeResources(NativeDrawContext* nvg) {
  if (!nvg) return;

  // Set up visual style for this plugin
  _drawingProperties.setProperty("mark", ml::colorToMatrix({ 0.01, 0.01, 0.01, 1.0 }));
  _drawingProperties.setProperty("mark_bright", ml::colorToMatrix({ 0.9, 0.9, 0.9, 1.0 }));
  _drawingProperties.setProperty("background", ml::colorToMatrix({ 0.6, 0.7, 0.8, 1.0 }));
  _drawingProperties.setProperty("common_stroke_width", 1 / 32.f);

  // TODO: Should these just be macros?
  // Centralized typography
  _drawingProperties.setProperty("title_text_size", 0.3f);
  _drawingProperties.setProperty("label_text_size", 0.4f);
  _drawingProperties.setProperty("dial_text_size", 0.5f);

  // Centralized layout settings for symmetrical positioning
  _drawingProperties.setProperty("grid_width", 10.0f);
  _drawingProperties.setProperty("grid_height", 7.0f);

  // Row positions (equidistant from center)
  _drawingProperties.setProperty("top_row_y", 0.4f);
  _drawingProperties.setProperty("bottom_row_y", 2.5f);

  // Column positions (equidistant from center)
  _drawingProperties.setProperty("left_col_x", 0.0f);
  _drawingProperties.setProperty("right_col_x", 5.0f);
  _drawingProperties.setProperty("adsr_col1_x", 0.0f);
  _drawingProperties.setProperty("adsr_col2_x", 2.5f);
  _drawingProperties.setProperty("adsr_col3_x", 5.0f);
  _drawingProperties.setProperty("adsr_col4_x", 7.5f);

  // Widget dimensions
  _drawingProperties.setProperty("large_dial_width", 5.0f);
  _drawingProperties.setProperty("large_dial_height", 2.5f);
  _drawingProperties.setProperty("small_dial_width", 2.5f);
  _drawingProperties.setProperty("small_dial_height", 2.5f);

  // Load embedded fonts (essential for text to work properly)
  // These fonts are embedded as C arrays and loaded directly from memory
  _resources.fonts["d_din"] = std::make_unique<ml::FontResource>(nvg, "d_din", resources::D_DIN_otf, resources::D_DIN_otf_size, 0);
  _resources.fonts["d_din_italic"] = std::make_unique<ml::FontResource>(nvg, "d_din_italic", resources::D_DIN_Italic_otf, resources::D_DIN_Italic_otf_size, 0);

  // // Helpful for debugging layout
  // _drawingProperties.setProperty("draw_widget_bounds", true);

}
