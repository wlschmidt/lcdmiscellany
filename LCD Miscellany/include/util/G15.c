// Functions that have to do with managing/detecting primary display devices,
// Not just G15s.

#import <constants.h>

function IsG19Installed() {
	return size(GetG15s(SDK_320_240_32));
}

function HasAllDirections($keyboard) {
	$state = GetDeviceState($keyboard);
	return ($state.type == SDK_320_240_32 || $state.type == LCD_G19);
}

// If left/right or up/down are both pressed, returns 0.
// Can specify individual keyboard to check, if so desired.
function FilterButton($button, $g15id) {
	if ($button & 0x33) {
		if (G15GetButtonsState($g15id) & (($button >> 1) + ($button << 1)))
			return 0;
	}
	return $button;
}

function GetMaxRes() {
	$devs = GetDeviceState(0);
	while ($i < size($devs)) {
		$dev = $devs[$i];
		if ($w < $dev.width) {
			$w = $dev.width;
		}
		if ($h < $dev.height) {
			$h = $dev.height;
		}
		if ($bpp < $dev.bpp) {
			$bpp = $dev.bpp;
		}
		$i++;
	}
	return list($w, $h, $bpp);
}

function GetMinRes() {
	$devs = GetDeviceState(0);
	$w = 1000000;
	$h = 1000000;
	$bpp = 32;
	while ($i < size($devs)) {
		$dev = $devs[$i];
		if ($w > $dev.width) {
			$w = $dev.width;
		}
		if ($h > $dev.height) {
			$h = $dev.height;
		}
		if ($bpp > $dev.bpp) {
			$bpp = $dev.bpp;
		}
		$i++;
	}
	return list($w, $h, $bpp);
}

