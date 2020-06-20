#ifndef LIBPX_EDITOR_APP_STATE_HPP
#define LIBPX_EDITOR_APP_STATE_HPP

namespace px {

class App;
class Log;
class MenuBar;
class Platform;

/// This is the base class for a state
/// that the app can be in.
class AppState
{
  /// A pointer to the app instance.
  App* app = nullptr;
public:
  /// Constructs a new base app state.
  ///
  /// @param app_ A pointer to the app hosting the state.
  AppState(App* app_) : app(app_) {}
  /// Just a stub.
  virtual ~AppState() {}
  /// Called on every frame to render the state.
  virtual void frame() = 0;
  /// Gets a pointer to the app.
  ///
  /// @return A pointer to the app, for non-const access.
  inline App* getApp() noexcept { return app; }
  /// Gets a pointer to the app.
  ///
  /// @return A pointer to the app, for const access.
  inline const App* getApp() const noexcept { return app; }
  /// Gets a pointer to the menu bar.
  ///
  /// @return A pointer to the menu bar, for non-const access.
  MenuBar* getMenuBar() noexcept;
  /// Gets a pointer to the menu bar.
  ///
  /// @return A pointer to the menu bar, for const access.
  const MenuBar* getMenuBar() const noexcept;
  /// Gets a pointer to the log.
  ///
  /// @return A pointer to the log, for non-const access.
  Log* getLog() noexcept;
  /// Gets a pointer to the log.
  ///
  /// @return A pointer to the log, for const access.
  const Log* getLog() const noexcept;
  /// Gets a pointer to the platform.
  ///
  /// @return A pointer to the platform.
  Platform* getPlatform() noexcept;
};

} // namespace px

#endif // LIBPX_EDITOR_APP_STATE_HPP
