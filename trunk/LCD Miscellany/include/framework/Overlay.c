#import <constants.h>
#requires <util\graphics.c>
#requires <util\text.c>
#requires <util\G15.c>
#requires <framework\theme.c>

struct MessageBoxOverlay {
	// Note:  Option 2 is cancel, and is automatically selected when a new message box is openned.
	var %prompt, %option1, %option2, %fxn1, %fxn2, %obj1, %obj2, %arg1, %arg2, %selected, %handler;
	var %fontIds;
	function MessageBoxOverlay($_handler, $_prompt, $_option1, $_option2, $_fxn1, $_fxn2, $_obj1, $_obj2, $_arg1, $_arg2) {
		%fontIds = RegisterThemeFontPair("OverlayFont");
		%handler = $_handler;

		%prompt = $_prompt;
		%option1 = $_option1;
		%option2 = $_option2;
		%fxn1 = $_fxn1;
		%fxn2 = $_fxn2;
		%obj1 = $_obj1;
		%obj2 = $_obj2;
		%arg1 = $_arg1;
		%arg2 = $_arg2;
		%selected = (size(%option2) != 0);
		$_handler.SetOverlay($this);
	}

	function G15ButtonDown($event, $param, $button) {
		$button = FilterButton($button);
		if ($button & 3) {
			if (size(%option2) && 3 != (G15GetButtonsState() & 3)) {
				if ($buttons & 1) {
					%selected ^= 1;
				}
				else {
					%selected ^= 1;
				}
				NeedRedraw();
			}
		}
		else if ($button == G15_CANCEL) {
			%Option2();
		}
		else if ($button == G15_OK) {
			if (%selected == 1)
				%Option2();
			else
				%Option1();
		}
		return 1;
	}

	function KeyDown($event, $param, $modifiers, $vk, $string) {
		if ($vk == VK_ESCAPE || $vk == VK_DELETE || $vk == VK_BACK) {
			%G15ButtonDown(,,8);
		}
		if ($vk == VK_RETURN) {
			%G15ButtonDown(,,4);
		}
		if ($vk == VK_RIGHT || $vk == VK_DOWN) {
			%G15ButtonDown(,,2);
		}
		if ($vk == VK_LEFT || $vk == VK_UP) {
			%G15ButtonDown(,,1);
		}
		return 1;
	}

	function Option1() {
		%handler.RemoveOverlay($this);
		if (IsObject(%obj1))
			%obj1.call(%fxn1, %arg1);
		else
			call(%fxn1, %arg1);
		NeedRedraw();
	}

	function Option2() {
		// Not needed when called from Close(), but can't hurt.
		%handler.RemoveOverlay($this);
		if (IsObject(%obj2))
			%obj2.call(%fxn2, %arg2);
		else
			call(%fxn2, %arg2);
		NeedRedraw();
	}

	// Aciton when forced to close by the event handler.
	function Close() {
		NeedRedraw();
		%Option2();
	}

	function Draw($event, $param, $name, $res) {
		$highRes = IsHighRes(@$res);
		UseThemeFont(%fontIds[$highRes]);

		$w = $res[0];
		$h = $res[1]-1;

		$cx = $w/2;
		$cy = $h/2;

		if ($highRes) {
			$vmargins = 4;
			$hmargins = 4;
			$bmargins = 4;
		}
		else {
			$vmargins = 2;
			$hmargins = 4;
			$bmargins = 2;
		}

		$fh = GetFontHeight();
		$middleGap = $fh/2;

		$fixedPrompt = Elipsisify(%prompt, $w - 2 * ($fh + $hmargins));
		$box = TextSize($fixedPrompt);
		$tw = $box[0];
		$th = $box[1];

		$bodyHeight = 2*$bmargins + $middleGap + $th + $fh;
		$bodyWidth = $tw;

		$o1w = TextSize(%option1)[0];
		$o2w = TextSize(%option2)[0];

		if (2*($o1w + 5*$bmargins/2) > $bodyWidth) {
			$bodyWidth = 2*($o1w + 5*$bmargins/2);
		}
		if ($o2w + 5*$bmargins/2 > $bodyWidth) {
			$bodyWidth = 2*($o2w + 5*$bmargins/2);
		}

		$top = $cy - $bodyHeight/2;
		$bottom = $top + $bodyHeight;
		$left = $cx - $bodyWidth/2;
		$right = $left + $bodyWidth;

		DoubleBox($left-$hmargins, $top-$vmargins, $right+$hmargins, $bottom+$vmargins);
		DisplayText($fixedPrompt, $cx - $tw/2, $top);

		$x1 = ($cx+$left-$o1w)/2;
		$y1 = ($bottom-$bmargins-$fh);

		DoubleBox($x1-$bmargins-1, $y1-$bmargins, $x1+$o1w+$bmargins-1, $y1+$fh+$vmargins-1);
		DisplayText(%option1, $x1, $y1);

		$x2 = ($cx+$right-$o2w)/2;

		DoubleBox($x2-$bmargins-1, $y1-$bmargins, $x2+$o2w+$bmargins-1, $y1+$fh+$vmargins-1);
		DisplayText(%option2, $x2, $y1);

		if (%selected == 0) {
			InvertRect($x1-$bmargins, $y1-$bmargins+1, $x1+$o1w+$bmargins-2, $y1+$fh+$vmargins-2);
		}
		else {
			InvertRect($x2-$bmargins, $y1-$bmargins+1, $x2+$o2w+$bmargins-2, $y1+$fh+$vmargins-2);
		}
	}
};


struct TimedOverlay {
	var %font, %prompt, %handler, %time, %timer;
	function TimedOverlay($_handler, $_prompt, $_time) {
		%time = $_time;
		%font = Font("04b03", 8);
		%handler = $_handler;
		$_handler.SetOverlay($this);
		%prompt = $_prompt;
		%timer = CreateFastTimer("Close", $_time, 0, $this);
	}

	function Close() {
		return %G15ButtonDown();
	}

	function G15ButtonDown() {
		if (%timer) {
			KillTimer(%timer);
			%timer = 0;
			%handler.RemoveOverlay($this);
			return 1;
		}
	}


	function Draw() {
		UseFont(%font);
		$box = TextSize(%prompt);
		$width = $box[0];
		$height = $box[1];
		$left = 80-$width/2-4;
		$top = 21-$height/2-1;
		$right = 80+$width/2+4;
		$bottom = 21+$height/2+1;
		DoubleBox($left, $top, $right, $bottom);
		DisplayText(%prompt, 80-($width-1)/2, $top+2);
	}
};

// Note: Depends on eventHandler global variable.
function MakeTimedOverlay($prompt, $time) {
	return TimedOverlay(eventHandler, $prompt, $time);
}

// Note: Depends on eventHandler global variable.
function MakeMessageBoxOverlay() {
	return TimedOverlay(eventHandler, @$);
}
