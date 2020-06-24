#ifndef LIBPX_EDITOR_MENU_BAR_HPP
#define LIBPX_EDITOR_MENU_BAR_HPP

namespace px {

/// Represents the menu bar at the top of the window.
class MenuBar final
{
  /// Contains the state information
  /// on what is visible and what is not.
  struct VisibilityState final
  {
    /// Whether or not the draw panel is visible.
    bool drawPanel = true;
    /// Whether or not the layer panel is visible.
    bool layerPanel = true;
    /// Whether or not the document properties are visible.
    bool docProperties = false;
    /// Whether or not the log is visible.
    bool log = false;
    /// The style editor widget.
    /// There's not a checkbox for this,
    /// since it's not useful for drawing.
    bool styleEditor = false;
  };
  /// The one and only instance of @ref VisibilityState.
  VisibilityState visibility;
  /// Indicates the current theme being used.
  const char* currentTheme = "Dark";
public:
  /// Enumerates the observable events in the menu bar.
  enum class Event
  {
    ClickedClose,
    ClickedExportCurrentFrame,
    ClickedExportPx,
    ClickedExportSpriteSheet,
    ClickedExportZip,
    ClickedRedo,
    ClickedSave,
    ClickedUndo,
    ClickedQuit,
    ClickedCustomTheme,
    ClickedTheme,
    ClickedZoomIn,
    ClickedZoomOut
  };
  /// Used for observing events from the menu bar.
  class Observer
  {
  public:
    /// Just a stub.
    virtual ~Observer() {}
    /// Observes an event from the menu bar.
    virtual void observe(Event) = 0;
  };
  /// Renders a frame of the menu bar.
  void frame(Observer* observer = nullptr);
  /// Indicates whether or not the draw panel should be visible.
  inline bool drawPanelVisible() const noexcept
  {
    return visibility.drawPanel;
  }
  /// Whether or not the layer panel should be visible.
  inline bool layerPanelVisible() const noexcept
  {
    return visibility.layerPanel;
  }
  /// Indicates whether or not the document properties should be visible.
  inline bool documentPropertiesVisible() const noexcept
  {
    return visibility.docProperties;
  }
  /// Indicates whether or not the log is visible.
  inline bool logVisible() const noexcept
  {
    return visibility.log;
  }
  /// Indicates whether or not the style editor is visible.
  inline bool styleEditorVisible() const noexcept
  {
    return visibility.styleEditor;
  }
  /// Gets the name of the currently selected theme.
  inline const char* getSelectedTheme() const noexcept
  {
    return currentTheme;
  }

  /// Sets the visibility of the document properties panel.
  ///
  /// @param state The visibility state of the document properties panel.
  inline void setDocumentPropertiesVisibility(bool state)
  {
    visibility.docProperties = state;
  }
protected:
  /// Renders the file menu.
  void fileMenu(Observer*);
  /// Renders the edit menu.
  void editMenu(Observer*);
  /// Renders the view menu.
  void viewMenu(Observer*);
};

} // namespace px

#endif // LIBPX_EDITOR_MENU_BAR_HPP
