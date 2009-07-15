#import <constants.h>
#requires <util\G15.c>
#import <list.c>
#requires <framework\Overlay.c>

struct EventHandler extends ObjectList {
	var %overlay;
	function EventHandler() {
		%ObjectList();
		SetEventHandler("G15ButtonDown", "HandleEvent", $this);
		SetEventHandler("Draw", "DrawEvent", $this);
		SetEventHandler("KeyUp", "HandleEvent", $this);
		SetEventHandler("KeyDown", "HandleEvent", $this);
		SetEventHandler("WMUser", "WMUser", $this);
		SetEventHandler("CounterUpdate", "CounterUpdate", $this);
		SetEventHandler("AudioChange", "HandleEvent", $this);
		SetEventHandler("Quit", "Quit", $this);
	}

	function CounterUpdate() {
		NeedRedrawIcons();
		$i = size(%objs);
		while ($i--) {
			if (%objs[$i].CounterUpdate(@$) | %objs[$i].HandleEvent(@$)) break;
		}
	}

	function WMUser($event, $param, $wParam, $lParam) {
		if ($wParam == 0 && $lParam == 0) {
			// Allows other apps to force a redraw.
			NeedRedraw();
		}
		else {
			%HandleEvent(@$);
		}
	}

	function Quit() {
		%HandleEvent("Quit");
		%objs = list();
	}

	// Used by either non-overlays to kick out an overlay, or by a new overlay to
	// replace an old overlay.  Should not be called by an overlay to remove itself,
	// unless it doesn't mind the call to Close().
	function SetOverlay($newOverlay) {
		NeedRedraw();
		if (!IsNull(%overlay)) {
			// Just in case.
			$oldOverlay = %overlay;
			%overlay = null;

			%Remove($oldOverlay);

			// An overlay should *NOT* call SetOverlay in response to this event.
			$oldOverlay.Close();
		}
		if (IsObject($newOverlay)) {
			%overlay = $newOverlay;
			%Insert(-1, $newOverlay);
		}
	}

	// Intended for overlays to use to remove themselves.  Does not call Close().
	function RemoveOverlay($oldOverlay) {
		if (Equals($oldOverlay, %overlay)) {
			NeedRedraw();
			%Remove($oldOverlay);
			%overlay = null;
		}
	}

	function HandleEvent($event, $param, $buttons) {
		$i = size(%objs);
		while ($i--) {
			if (%objs[$i].call($event, $) | %objs[$i].HandleEvent(@$)) break;
		}
	}

	function DrawEvent($event, $param) {
		$len = size(%objs);
		SetBgColor(colorBg);
		SetDrawColor(colorText);
		while ($i < $len) {
			if (%objs[$i].call($event, $)) break;
			$i++;
		}
	}
};


struct MenuHandler {
	var %eventHandler;
	var %views, %currentView;
	var %toolbarVisible;
	var %hideTimer;
	function MenuHandler($Handler) {
		%views = list();
		%eventHandler = $Handler;
		%eventHandler.Insert(0, $this);
		$[0] = 0;
		%AddView(@$);
		%toolbarVisible = 0;
	}

	function AddView($index/*, view1, view2,...*/) {
		for ($i=1; $i<size($);) {
			if (!IsObject($[$i])) {
				pop($, $i);
				continue;
			}
			$[$i].eventHandler = %eventHandler;
			$i++;
		}
		if (size($) < 2) return;
		if ($index >= %currentView) {
			if (!size(%views)) {
				%currentView = size(%views);
				insert(%views, @$);
				%SetView(0);
				return;
			}
			$index += size(%currentView);
		}
		insert(%views, @$);
	}

	function SetView($val) {
		if (IsObject($val)) {
			while ($i < size(%views)) {
				if (Equals(%views[$i], $val)) break;
				$i++;
			}
			$val = $i;
		}
		if (IsInt($val) && $val >= 0 && $val < size(%views)) {
			if (%currentView < size(%views)) {
				%views[%currentView].Hide();
				%eventHandler.Remove(%views[%currentView]);
			}
			%currentView = $val;
			%eventHandler.Insert(0, %views[%currentView]);
			%views[%currentView].Show();
		}
		NeedRedraw();
	}

	function Remove(/*obj1, obj2, etc*/) {
		for ($i=0; $i<size($); $i++) {
			listRemove(%views, $[$i]);
		}
	}

	function G15ButtonDown($event, $param, $button) {
		$button = FilterButton($button);
		if (($button & 0x4F) && !%views[%currentView].hasFocus) {
			if ($button & 3) {
				%RestartHideTimer();
				%toolbarVisible = 1;
				if ($button == 1) {
					$newViewOffset = size(%views)-1;
				}
				else {
					$newViewOffset = 1;
				}
				%SetView(($newViewOffset+%currentView) % size(%views));
			}
			else if ($button == G15_MENU) {
				// Doesn't hurt to call it when not needed.
				%RestartHideTimer();
				%toolbarVisible ^= 1;
			}
			else if ($button == G15_OK) {
				%views[%currentView].Focus();
				%toolbarVisible = 0;
			}
			else if ($button == G15_CANCEL) {
				if (!%toolbarVisible) {
					MessageBoxOverlay(%eventHandler, "Quit LCD Miscellany?|n(" +s GetVersionString(1) +s ")", "Yes", "No", "Quit", null);
				}
				%toolbarVisible = 0;
			}
			NeedRedraw();
			return 1;
		}
	}

	function RestartHideTimer() {
		if (!%hideTimer) {
			%StartHideTimer();
		}
		else %hideTimer = GetTickCount () + 3500;
	}

	function StartHideTimer() {
		if (!%hideTimer) {
			%hideTimer = GetTickCount () + 3500;
			CreateTimer("AutoHide", 1, 0, $this);
		}
	}

	function AutoHide($id) {
		if (GetTickCount() - %hideTimer >= 0 || %hideTimer == 0) {
			KillTimer($id);
			%hideTimer = 0;
			%toolbarVisible = 0;
			NeedRedraw();
		}

	}

	function Draw($event, $param, $name, $res) {
		if (%toolbarVisible) {
			$width = $res[0];
			$height = $res[1]-1;
			$iconHeight = $height-7;

			SetBgColor(colorBg);
			SetDrawColor(colorText);

			ClearRect(0,$iconHeight-2, $width, $height);
			DrawRect(0,$iconHeight-2,$width,$iconHeight-2);
			$len = 13 * size(%views);
			if ($len <= $width)
				$pos = $width/2 - $len/2;
			else {
				$pos = $width/2-6 - 13*%currentView;
				if ($pos > 0) $pos = 0;
				else if ($pos < $width-$len) $pos = $width-$len;
			}
			for (; $i<size(%views); $i++) {
				DrawImage(%views[$i].toolbarImage, $pos, $iconHeight);
				if (%currentView == $i) {
					InvertRect($pos, $iconHeight, $pos+11, $height);
				}
				$pos+=13;
			}
		}
	}
};




