#pragma once

namespace flybrain {

/**
 * @brief Enforces Title Override mode to prevent OOM panics on Switch HOS.
 *
 * Checks appletGetAppletType() on Nintendo Switch. If running in LibraryApplet mode
 * (Album), halts initialization, displays an instructional warning screen, and prompts
 * the user to launch with Title Override.
 *
 * @return true if Title Override / Application mode is active (safe to allocate connectome).
 * @return false if launched via Album (halted and exited safely).
 */
class AppletGuard {
public:
    static bool enforceTitleOverride();
};

} // namespace flybrain
