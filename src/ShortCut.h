#ifndef SHORTCUT_H
#define SHORTCUT_H

#include "global.h"

struct Shortcut {
	unsigned int flags;
	unsigned int vkey;
	unsigned char *command;
};

class ShortcutManager {
	int numShortcuts;
	Shortcut* shortcuts;
public:
	void ClearShortcuts();
	inline ShortcutManager(): numShortcuts(0), shortcuts(0) {}
	void Load();
	void HotkeyPressed(WPARAM id);
	inline ~ShortcutManager() {
		ClearShortcuts();
	}
};

extern ShortcutManager shortcutManager;

#endif
