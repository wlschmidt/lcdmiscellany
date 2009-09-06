function SetGxxStates($id, $MLights, $Light, $LCDLight, $LCDColor) {
	G15SetBacklightColor($LCDColor, $id);
	Wait(10);
	G15SetBacklightColor($LCDColor, $id);
	Wait(10);
	G15SetLight($Light, $id);
	Wait(10);
	G15SetLight($Light, $id);
	Wait(10);
	G15SetLCDLight($LCDLight, $id);
	Wait(10);
	G15SetLCDLight($LCDLight, $id);
	Wait(10);
	G15SetMLights($MLights, $id);
	Wait(10);
	G15SetMLights($MLights, $id);
}

function SetG15State($id, $MLights, $Light, $LCDLight) {
	G15SetLight($Light, $id);
	Wait(10);
	G15SetLight($Light, $id);
	Wait(10);
	G15SetLCDLight($LCDLight, $id);
	Wait(10);
	G15SetLCDLight($LCDLight, $id);
	Wait(10);
	G15SetMLights($MLights, $id);
	Wait(10);
	G15SetMLights($MLights, $id);
}

function SetG19State($id, $MLights, $LCDColor) {
	G15SetBacklightColor($LCDColor, $id);
	Wait(10);
	G15SetBacklightColor($LCDColor, $id);
	Wait(10);
	G15SetMLights($MLights, $id);
	Wait(10);
	G15SetMLights($MLights, $id);
}

function BackupLightStates() {
	if (!gLightBackupCount) gBackupLightStates = list();
	gBackupLightStates[gLightBackupCount] = GetDeviceState();
	gLightBackupCount++;
}

function RestoreLightStates() {
	if (gLightBackupCount) gLightBackupCount--;
	else return;
	for ($i=0; $i<size(gBackupLightStates[gLightBackupCount]); $i++) {
		$state = gBackupLightStates[gLightBackupCount][$i];
		$type = $state.type;
		if ($type == LCD_G11 || $type == LCD_G15_V1 || $type == LCD_G15_V2 || $type == LCD_Z10 || $type == LCD_GAME_PANEL) {
			SetG15State($state.name, $state.mkeys, $state.LCDColor, $state.LCDLight);
		}
		else if ($type == LCD_G19) {
			SetG19State($state.name, $state.mkeys, $state.LCDColor);
		}
	}
	if (!gLightBackupCount) gBackupLightStates = null;
}

struct ScreenSaverLightToggle {
	var %saving, %enabled;
	function ScreenSaverLightToggle($_eventHandler) {
		$_eventHandler.Insert(-1, $this);
		SetEventHandler("ScreenSaver", "HandleEvent", $_eventHandler);
		SetEventHandler("MonitorOff", "HandleEvent", $_eventHandler);
		//AddConfig(CONFIG_CHECKBOX, "Turn off lights on saver", $this, "SetMode", "SaverLightToggle");
		%enabled = 1;
	}

	function MonitorOff($event, $param, $starting) {
		%ScreenSaver($event, $param, $starting);
	}

	function ScreenSaver($event, $param, $starting) {
		// Fix for switching from screensaver to power save mode.
		if (%saving) {
			Wait(5000);
		}
		$power = SystemState();
		$starting = $power.screenSaverOn | $power.monitorOff;
		if (%enabled) {
			if ($starting) {
				if (!%saving) {
					BackupLightStates();
					SetG15State("", 0,0,0);
					SetG19State("", 0,0);
				}
			}
			else if (%saving) {
				RestoreLightStates();
			}
		}
		%saving = $starting;
	};

	function SetMode($on) {
		if (%saving && $on != %enabled) {
			%ScreenSaver();
			%enabled = $on;
			%ScreenSaver(0,0,1);
		}
		%enabled = $on;
	}
}

function LightAlert($blinks, $mainLight, $lcdLight, $mLights, $lcdColor) {
	if ($blinks <= 0) return;

	$old = gAlertState[0];
	gAlertState = list(2*$blinks, $mainLight, $lcdLight, $mLights, $lcdColor);
	if ($old > 0) {
		return;
	}
	BackupLightStates();
	CreateFastTimer("LightAlerter", 500, 1);
}

function LightAlerter($timer) {
	gAlertState[0]--;
	if (gAlertState[0] == 0) {
		RestoreLightStates();
		KillTimer($timer);
		gAlertState = null;
		return;
	}
	if ((gAlertState[0] & 1) == 0) {
		RestoreLightStates();
		BackupLightStates();
		return;
	}
	SetGxxStates(, gAlertState[1], gAlertState[2], gAlertState[3], gAlertState[4]);
}

