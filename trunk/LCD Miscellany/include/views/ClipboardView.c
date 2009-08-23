#import <Views\View.c>
#requires <util\Text.c>

struct ClipboardView extends View {
	var %data, %data2, %lines, %offset, %happy, %zoom, %x, %y;

	// Despite name, used when fail to read clipboard data to try again
	// every second.
	var %redrawTimer;

	// Don't zoom when can't.
	var %minZoom;

	// formatting dimensions.
	var %w, %h, %bpp;

	function ClipboardView() {
		%InitFonts();
		%toolbarImage = LoadImage("Images\Clipboard.png");

		%noDrawOnCounterUpdate = 1;
		%noDrawOnAudioChange = 1;
    }


	function Draw($event, $param, $name, $res) {
		$farRight = $right = $res[0]-1;
		$bottom = $res[1]-1;
		$bp = $res[2];
		$highRes = IsScreenHighRes(@$res);

		if (!%happy) %Update();
		ClearScreen();
		if (IsString(%data)) {
			UseFont(GetThemeFont(%fontIds[$highRes]));
			DisplayText(%data, $highRes, %offset * GetFontHeight());
		}
		else if (type(%data) ==S "Image") {
			DrawImage(%data, %x, %y);
		}
		else if (type(%data) ==S "Image32") {
			if ($highRes) {
				DrawImage(%data, %x, %y);
			}
			else {
				// Making some assumptions here about relative size.
				// May go through the math to do it right later.  Or not.
				// $s = %data.Size();
				$x2 = (%x - %w/2)/2 + $res[0]/2;
				$y2 = (%y - %h/2)/2 + $res[1]/2;
				DrawImage(%data2, $x2, $y2);
			}
		}
	}

	function UpdateDimensions() {
		$res = GetMaxRes();
		%w = $res[0];
		%h = $res[1];
		%bpp = $res[2];
		return $res;
	}

	function KeyDown($event, $param, $modifiers, $vk) {
		$vk = MediaFlip($vk);
		$highRes = IsScreenHighRes(@GetMaxRes());
		if (%hasFocus || IsNull($event)) {
			if (IsString(%data)) {
				UseFont(GetThemeFont(%fontIds[$highRes]));
				$height = GetFontHeight();
				// Volume up.
				if ($vk == 0xAE || $vk == VK_UP) {
					if (%offset < 0) {
						%offset ++;
					}
				}
				// Volume down.
				else if ($vk == 0xAF || $vk == VK_DOWN) {
					if (%offset > %h/$height-%lines) {
						%offset --;
					}
				}
			}
			else if (type(%data) ==S "Image" || type(%data) ==S "Image32") {
				// Volume up.
				if ($vk == 0xAE) {
					if (!%minZoom)
						%ReZoom(%zoom / 2.0);
				}
				// Volume down.
				else if ($vk == 0xAF) {
					if (%zoom != 1)
						%ReZoom(%zoom * 2.0);
				}
				else if ($vk == 0x25 || $vk == 0x27) {
					$width = %data.Size()[0];
					if ($width > %w) {
						%x += 0x26 - $vk;
						if (%x > 0) {
							%x = 0;
						}
						else if (%x < %w - $width) {
							%x = %w - $width;
						}
					}
				}
				else if ($vk == 0x26 || $vk == 0x28) {
					$height = %data.Size()[1];
					if ($height > %h) {
						%y += 0x27 - $vk;
						if (%y > 0) {
							%y = 0;
						}
						else if (%y < %h - $height) {
							%y = %h - $height;
						}
					}
				}
			}
			NeedRedraw();
			return 1;
		}
	}

	function Focus() {
		if (!%hasFocus) {
			%hasFocus = 1;
			RegisterKeyEvent(0, 0xAE, 0xAF, 0x25, 0x26, 0x27, 0x28);
		}
	}

	function Unfocus() {
		if (%hasFocus) {
			%hasFocus = 0;
			UnregisterKeyEvent(0, 0xAE, 0xAF, 0x25, 0x26, 0x27, 0x28);
		}
	}

	function G15ButtonDown($event, $param, $button) {
		$button = FilterButton($button);
		if ($button & 0x3F) {
			if ($button & 0x33) {

				if ($button == G15_LEFT) {
					$key = VK_LEFT;
				}
				else if ($button == G15_RIGHT) {
					$key = VK_RIGHT;
				}
				else if ($button == G15_UP) {
					$key = VK_UP;
				}
				else if ($button == G15_DOWN) {
					$key = VK_DOWN;
				}
				$hp = 1;
				if (!IsString(%data)) {
					$hp = %h;
				}
				while ($hp >= 0) {
					%KeyDown(,,,$key);
					$hp -= 30;
				}
			}
			else if ($button == G15_CANCEL) {
				%Unfocus();
			}
			else if ($button == G15_OK) {
				if (%zoom != 1)
					%ReZoom(%zoom * 2.0);
			}
			NeedRedraw();
			return 1;
		}
	}

	function ZoomImage($image, $newZoom) {
		$dim = $image.Size();
		$width = $dim[0];
		$height = $dim[1];

		%minZoom = 0;
		if ($newZoom * $width < %w && $newZoom*$height < %h) {
			$newZoom = %w*1.0/$width;
			if ($newZoom * $height > %h) {
				$newZoom = %h *1.0/$height;
			}
			// Safety margin.
			$newZoom *= 0.9999;
			%minZoom = 1;
		}
		if ($newZoom > 1 || $newZoom <= 0) $newZoom = 1;

		%zoom = $newZoom;
		$highRes = IsScreenHighRes(@GetMaxRes());
		if ($highRes) {
			%data = $image.Zoom(%zoom);
			%data2 = %data.ToImage(0.5, -1);
		}
		else {
			%data2 = null;
			%data = %data2.ToImage(%zoom, -1);
		}
	}

	function ReZoom($newZoom) {
		%UpdateDimensions();
		$dim = %data.Size();
		$width  = $dim[0];
		$height = $dim[1];
		$temp = GetClipboardData();
		if (type($temp[1]) !=S "Image32") {
			return %Update();
		}
		%ZoomImage($temp[1], $newZoom);
		$dim = %data.Size();
		$width2  = $dim[0];
		$height2 = $dim[1];
		if ($width2 < %w) {
			%x = (%w-$width2)/2;
		}
		else {
			$center = %x - %w/2;
			%x = $width2*(1.0*$center)/$width + %w/2;
			if (%x > 0) %x = 0;
			if (%x < %w-1-$width2) %x = %w-1-$width2;
		}
		if ($height2 < %h) {
			%y = (%h-$height2)/2;
		}
		else {
			$center = %y - %h/2;
			%y = $height2*(1.0*$center)/$height + %h/2;
			if (%y > 0) %y = 0;
			if (%y < %h-1-$height2) %y = %h-1-$height2;
		}
	}

	function Update() {
		%offset = 0;
		%happy = 0;
		%data = null;
		%data2 = null;
		$highRes = IsScreenHighRes(@%UpdateDimensions());
		UseFont(GetThemeFont(%fontIds[$highRes]));

		for ($i=0; $i<3; $i++) {
			$temp = GetClipboardData();
			if (IsList($temp)) {
				%happy = 1;
				if (IsString($temp[1])) {
					%data = FormatText($temp[1], %w);
					%lines = TextSize(%data)[1]/GetFontHeight();
				}
				else if (type($temp[1]) ==S "Image32") {
					%ZoomImage($temp[1], 0);

					$dim = %data.Size();
					$width = $dim[0];
					$height = $dim[1];

					%x = (%w-$width)/2;
					%y = (%h-$height)/2;
				}
				break;
			}
			if (!IsNull($temp)) {
				%data = "[No Recognized Format]";
				if ($i > 1) break;
			}
			else {
				// Reduces chance of ending up with an error message.
				%data = "[Error Reading Clipboard]";
			}
			Sleep(10);
		}
		NeedRedraw();
		if (%happy) {
			if (%redrawTimer) {
				KillTimer(%redrawTimer);
				%redrawTimer = 0;
			}
		}
		else if (!%redrawTimer) {
			%redrawTimer = CreateTimer("NeedRedraw", 1);
		}
	}

	function Show() {
		SetEventHandler("ClipboardChange", "Update", $this);
	}

	function Hide() {
		if (%redrawTimer) {
			KillTimer(%redrawTimer);
			%redrawTimer = 0;
		}
		%happy = 0;
		%data = null;
		%data2 = null;
		SetEventHandler(, "ClipboardChange");
	}
};

