#include "applet_guard.hpp"
#include <iostream>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace flybrain {

bool AppletGuard::enforceTitleOverride() {
#ifdef __SWITCH__
    AppletType at = appletGetAppletType();

    // Check if launched as an application (Title Override or NSP forwarder)
    if (at == AppletType_Application || at == AppletType_SystemApplication) {
        return true; // Full Application memory pool available!
    }

    // Otherwise, we are running in LibraryApplet / Album mode with restricted memory!
    // Initialize standard console to inform the user
    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("\x1b[2J"); // Clear screen
    printf("\x1b[1;36m=================================================================\x1b[0m\n");
    printf("\x1b[1;33m             flybrain-nx: TITLE OVERRIDE REQUIRED                \x1b[0m\n");
    printf("\x1b[1;36m=================================================================\x1b[0m\n\n");

    printf("\x1b[1;37m You have launched flybrain-nx in \x1b[1;31mApplet Mode (Album)\x1b[1;37m.\x1b[0m\n\n");
    printf(" In Applet Mode, Horizon OS restricts available memory to ~200 MB.\n");
    printf(" Loading the complete Drosophila melanogaster brain connectome\n");
    printf(" requires full Application memory (~357 MB allocated safely).\n\n");

    printf("\x1b[1;32m ---------------------------------------------------------------\x1b[0m\n");
    printf("\x1b[1;32m  HOW TO LAUNCH WITH TITLE OVERRIDE:\x1b[0m\n");
    printf("   1. Return to the Nintendo Switch HOME Menu.\n");
    printf("   2. Hold the \x1b[1;33m[R] button\x1b[0m on your controller.\n");
    printf("   3. While holding [R], select and launch \x1b[1;33mANY installed game\x1b[0m or demo.\n");
    printf("   4. Keep holding [R] until the Homebrew Menu appears.\n");
    printf("   5. Launch \x1b[1;36mflybrain-nx\x1b[0m from the Homebrew Menu.\n");
    printf("\x1b[1;32m ---------------------------------------------------------------\x1b[0m\n\n");

    printf(" Memory allocation has been prevented to avoid a fatal system panic.\n\n");
    printf("\x1b[1;35m Press (+) or the HOME Button to exit safely.\x1b[0m\n");

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) {
            break;
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return false;
#else
    // PC / Host testing environment always has full application memory
    return true;
#endif
}

} // namespace flybrain
