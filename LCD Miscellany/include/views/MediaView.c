#import <Views\View.c>
#import <constants.h>

#requires <util\G15.c>
#requires <util\time.c>
#requires <util\Text.c>
#requires <framework\Theme.c>

function DrawSpectrum2($cache, $specHist, $left, $top, $right, $bottom) {
	// Significant speed optimization.
	while (!Equals($cache[5][0], $specHist[5]) && size($cache)) {
		pop($cache, 0);
	}

	$colors = list(0,0,0,0,0,RGB(0,255,0), RGB(0,220,0), RGB(0,170,0), RGB(0,120,0), RGB(255,0,0));
	for ($i=5; $i <= 9; $i++) {
		// Draws at offset instead of above/below.
		//$top2 = $top - 20*(9-$i);
		//$bottom2 = $bottom - 20*(9-$i);
		//$left2 = $left + 6*(9-$i);
		$top2 = $top;
		$bottom2 = $bottom;
		$left2 = $left;

		$h = ($top2 - $bottom2)/255.0;
		$mul2 = (size($specHist[0])-1) / (1.0*($right-$left2));

		$waveform = $specHist[$i];

		if (!size($cache[$i][1])) {
			$points = list();

			for ($x=$right-$left2; $x>=0; $x--) {
				$val = $bottom2 + $h*$waveform[$mul2*$x];
				if ($val < $top2) $val = $top2;
				$points[2*$x] = $x + $left2;
				$points[2*$x+1] = $val;
			}
			$cache[$i] = list($waveform, $points);
		}
		ColorLine($colors[$i], @$cache[$i][1]);
	}
}

function DrawSpectrum($waveform, $left, $top, $right, $bottom) {
	$h = ($top - $bottom)/255.0;
	$mul2 = (size($waveform)-1) / (1.0*($right-$left));
	for ($x=$left; $x<=$right; $x++) {
		$val = $waveform[$mul2*($x-$left)];
		if ($val > 255) $val = 255;
		DrawRect($x, $bottom, $x, $bottom + $h*$val);
	}
}

function DrawWave($waveform, $left, $top, $right, $bottom) {
	$mul = ($bottom - $top)/255.0;
	$mid = ($top+$bottom)/2.0 + $mul * 0.5;
	$mul2 = (size($waveform)-1) / (1.0*($right-$left));
	for ($x=$left; $x<=$right; $x++) {
		DrawRect($x, $mid, $x, $mid + $mul*$waveform[$mul2*($x-$left)]);
	}
}

struct MediaView extends View {
	// %selected is a control index, %player is a player index.
	var %players, %player, %selected;
	var %titleScroller, %artistScroller;
	var %bigTitleScroller, %bigArtistScroller;
	var %scrollTimer;
	var %bump;
	var %specCache;

	function MediaView () {
		%specCache = list();
		%players = list();

		%InitFonts();
		%InitImages();

		%fontIds = list(@%fontIds, @RegisterThemeFontPair(type($this) +s "Title"));

		for ($i=0; $i<size($); $i++) {
			if (!IsNull($[$i])) %players[size(%players)] = $[$i];
		}
		%selected = -1;

		%noDrawOnCounterUpdate = 1;
		%noDrawOnAudioChange = 1;
	}

	function Focus() {
		if (!%hasFocus) {
			%selected = -1;
			%hasFocus = 1;
		}
	}

	function Bump() {
		%bump++;
		if (!(%bump%2)) {
			if (%titleScroller.Bump() || %bigTitleScroller.Bump()) {
				NeedRedraw();
			}
		}
		if (!(%bump%3)) {
			if (%artistScroller.Bump() || %bigArtistScroller.Bump()) {
				NeedRedraw();
			}
			else if (!(%bump%6)) {
				NeedRedraw();
			}
		}
	}

	function Show() {
		if (!%scrollTimer) {
			%titleScroller = ManualScrollingText(,117,1,1,18);
			%artistScroller = ManualScrollingText(,117,1,1,12);
			%bigTitleScroller = ManualScrollingText(,320,1,1,28);
			%bigArtistScroller = ManualScrollingText(,320,1,1,22);
			%bump = 0;
			RegisterKeyEvent(0, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3);
			%players[%player].Update();
			// Can update $player twice, at times.  Not a huge deal.
			for ($i=0; $i < size(%players); $i++) {
				if ($i == %player) continue;
				%players[$i].Update();
				if (%players[$i].state > %players[%player].state)
					%player = $i;
			}
			%scrollTimer = CreateFastTimer("Bump", 60,,$this);
			%players[%player].Show();
		}
	}

	function Hide() {
		if (%scrollTimer) {
			KillTimer(%scrollTimer);
			%scrollTimer = 0;
			// Not really needed... save a few bytes ram.
			%titleScroller = 0;
			%artistScroller = 0;
			%bigTitleScroller = 0;
			%bigArtistScroller = 0;
			UnregisterKeyEvent(0, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3);
			%players[%player].Hide();
		}
	}

	function Unfocus() {
		if (%hasFocus) {
			%hasFocus = 0;
		}
	}

	function G15ButtonDown($event, $param, $button) {
		$button = FilterButton($button);
		if ($button & 0x3F) {
			if ($button & (G15_LEFT | G15_RIGHT)) {
				$p = %players[%player];
				$delta = 1;
				if ($button == G15_LEFT)
					$delta = -1;
				if (%selected == -1) {
					%players[%player].Hide();
					%player = (%player + size(%players)+$delta) % size(%players);
					%players[%player].Show();
				}
				else {
					%selected = (%selected + $delta + 3)%3;
					if ($p.noBalance && %selected == 2)
						%selected = (%selected + $delta + 3)%3;
				}
			}
			else if ($button == G15_OK) {
				if (%selected < 0)
					%selected = 0;
			}
			else if ($button == G15_CANCEL) {
				if (%selected >= 0)
					%selected = -1;
				else
					%Unfocus();
			}
			else if ($button & (G15_UP | G15_DOWN)) {
				$delta = 1;
				if ($button == G15_UP)
					$delta = -1;
				%players[%player].Hide();
				%player = (%player + size(%players)+$delta) % size(%players);
				%players[%player].Show();
			}
			NeedRedraw();
			return 1;
		}
	}

	function VolumeWheel($change) {
		$p = %players[%player];
		if ($p.state < 0) %selected = -1;
		if (%selected == 0) {
			if ($change < 0) {
				$p.Prev();
			}
			else {
				$p.Next();
			}
		}
		else if (%selected == 1) {
			$p.ChangeMode($change);
		}
		else if (%selected == 2) {
			$p.ChangeBalance($change);
		}
		else {
			$p.ChangeVolume($change);
		}
	}

	function KeyDown($event, $param, $modifiers, $vk) {
		if (!$modifiers) {
			if ($vk == 0xAE) {
				%VolumeWheel(-1);
			}
			else if ($vk == 0xAF) {
				%VolumeWheel(1);
			}
			else if ($vk == 0xB2) {
				%players[%player].Stop();
			}
			else if ($vk == 0xB3) {
				%players[%player].PlayPause();
			}
			else if ($vk == 0xB0) {
				%players[%player].Next();
			}
			else if ($vk == 0xB1) {
				%players[%player].Prev();
			}
			else if ($vk == 0xAD) {
				%players[%player].ToggleMute();
			}
			return 1;
		}
	}

	function DrawG15() {
		$p = %players[%player];
		$pos = 80 - 13 * size(%players)/2;
		for (; $i<size(%players); $i++) {
			DrawImage(%players[$i].mediaImage, $pos, 35);
			if (%player == $i) {
				InvertRect($pos, 35, $pos+11, 42);
			}
			$pos+=13;
		}

		$font = GetThemeFont(%fontIds[0]);
		$titleFont = GetThemeFont(%fontIds[2]);

		DisplayText("Volume:", 1, 28);
		DisplayText("Balance:", 69, 28);

		if ($p.state >= 0) {
			%artistScroller.SetText($p.artist);
			if (size($p.artist)) {
				%artistScroller.DisplayText(1, 0);

				UseFont($titleFont);
				%titleScroller.SetText($p.title);
				%titleScroller.DisplayText(1, 6);
			}
			else {
				UseFont($titleFont);
				%titleScroller.SetText($p.title);
				%titleScroller.DisplayText(1, 3);

			}
			UseFont($font);

			ClearRect(119,0,159,20);
			DisplayText("of", 80, 21);
			DisplayTextRight(FormatDuration($p.position), 65, 21);
			if ($p.duration <= 0 && $p.position) {
				$duration = "N/A";
			}
			else {
				$duration = FormatDuration($p.duration);
			}
			DisplayTextRight($duration, 119, 21);

			if ($p.muted) {
				DisplayText("M", 37, 28);
			}

			DisplayTextRight($p.volume +s "%", 62, 28);
			DisplayTextRight($p.balance, 119, 28);

			DisplayTextCentered($p.mode, 140,8);

			if ($p.tracks) {
				if ($p.tracks < 10000)
					DisplayTextCentered($p.track +s "/" +s $p.tracks, 140, 15);
				else
					DisplayTextCentered($p.track, 140, 15);
			}
			if (%selected >= 0) {
				if (!%selected) {
					if (size($p.artist))
						InvertRect(0,6,118,18);
					else
						InvertRect(0,3,118,15);
				}
				else if (%selected == 1) {
					$s = TextSize($p.mode)[0];
					InvertRect(139 - $s/2,7,141 + ($s+1)/2,13);
				}
				else if (%selected == 2) {
					InvertRect(68,27,119,33);
				}
			}
		}
		else {
			%selected = -1;
		}

		if ($p.state < 0) {
			DisplayText($p.playerName +s " Not Running", 1, 21);
		}
		else {
			DisplayText(list("Stopped:", "Paused:", "Playing:")[$p.state], 1, 21);
		}

		DrawRect(120, 0, 120, 34);
		DrawRect(0, 34, 159, 34);


		DisplayTextCentered("Playmode", 140, 1);
		InvertRect(121, 6, 159);

		InvertRect(121, 14, 159, 20);

		InvertRect(0,20,119,26);

		if (size($p.spectrum)) {
			DrawSpectrum($p.spectrum, 121, 21, 159, 34);
		}
		else if (size($p.waveform)) {
			DrawWave($p.waveform, 121, 21, 159, 33);
		}
	}

	function DrawG19($event, $param, $name, $res) {
		$p = %players[%player];

		$font = GetThemeFont(%fontIds[1]);
		$titleFont = GetThemeFont(%fontIds[3]);

		if (!IsNull($p.image)) {
			$imageRes = $p.image.Size();

			$h = 106;
			if ($imageRes[1] < $res[1]-$h) {
				// Vertical center if too short.
				$h += ($res[1]-$h - $imageRes[1])/2;
			}
			else {
				// Align bottom if too tall.
				$h = $res[1] - $imageRes[1];
			}
			DrawImage($p.image, ($res[0]-$imageRes[0])/2, $h);
		}
		else if (size($p.specHist)) {
			// Flash lights to music.  Could probably do much better.
			// G15SetBacklightColor(RGB($p.spectrum[20], $p.spectrum[160], $p.spectrum[300]));
			DrawSpectrum2(%specCache, $p.specHist, 0, 105, 319, 239);
		}
		else if (size($p.spectrum)) {
			DrawSpectrum($p.spectrum, 0, 106, 319, 239);
		}
		else if (size($p.waveform)) {
			DrawWave($p.waveform, 0, 106, 319, 239);
		}

		ClearRect(0,0,319,105);

		$pos = 160 - 18 * size(%players)/2;



		ColorRect(0, 86, 319, 87, colorHighlightBg);
		for ($i=0; $i<size(%players); $i++) {
			if (%player == $i) {
				ColorRect($pos, 88, $pos+17, 103, colorHighlightBg);
			}
			DrawImage(%players[$i].bigMediaImage, $pos+1, 88);
			$pos+=19;
		}
		ColorRect(0, 104, 319, 105, colorHighlightBg);

		UseFont($font);

		DisplayText("Volume:", 1, 66);
		DisplayText("Balance:", 120, 66);

		ColorRect(0, 46, 319, 64, colorHighlightBg);
		SetDrawColor(colorHighlightText);
		if ($p.state < 0) {
			DisplayText($p.playerName +s " Not Running", 1, 46);
		}
		else {
			DisplayText(list("Stopped:", "Paused:", "Playing:")[$p.state], 1, 46);
		}

		SetDrawColor(colorText);
		if ($p.state >= 0) {
			if (!%selected) {
				ColorRect(0, 0, 319, 44, colorHighlightBg);
				SetDrawColor(colorHighlightText);
			}

			%bigArtistScroller.SetText($p.artist);
			if (size($p.artist)) {
				%bigArtistScroller.DisplayText(1, 0);

				UseFont($titleFont);
				%bigTitleScroller.SetText($p.title);
				%bigTitleScroller.DisplayText(1, 20);
			}
			else {
				UseFont($titleFont);
				%bigTitleScroller.SetText($p.title);
				%bigTitleScroller.DisplayText(1, 10);

			}
			UseFont($font);

			SetDrawColor(colorHighlightText);

			DisplayText("of", 147, 46);
			DisplayTextRight(FormatDuration($p.position), 125, 46);
			if ($p.duration <= 0 && $p.position) {
				$duration = "N/A";
			}
			else {
				$duration = FormatDuration($p.duration);
			}
			DisplayTextRight($duration, 220, 46);

			if ($p.tracks) {
				if ($p.tracks < 10000)
					DisplayTextCentered($p.track +s "/" +s $p.tracks, 275, 46);
				else
					DisplayTextCentered($p.track, 275, 46);
			}

			SetDrawColor(colorText);

			$v = $p.volume +s "%";
			DisplayTextRight($v, 105, 66);
			if ($p.muted) {
				DrawLine(105-TextSize($v)[0], 74, 105, 74);
			}

			if (%selected == 2) {
				SetDrawColor(colorHighlightText);
				ColorRect(187, 66, 222, 84, colorHighlightBg);
				DisplayTextRight($p.balance, 220, 66);
				SetDrawColor(colorText);
			}
			else {
				DisplayTextRight($p.balance, 220, 66);
			}

			if (%selected == 1) {
				$s = TextSize($p.mode)[0];
				ColorRect(275 - $s/2,66,276 + ($s+1)/2, 84, colorHighlightBg);
				SetDrawColor(colorHighlightText);
			}
			DisplayTextCentered($p.mode, 275,66);
/*
			DisplayTextCentered($p.mode, 140,8);

			if (%selected >= 0) {
				if (!%selected) {
					if (size($p.artist))
						InvertRect(0,6,118,18);
					else
						InvertRect(0,3,118,15);
				}
				else if (%selected == 1) {
					$s = TextSize($p.mode)[0];
					InvertRect(139 - $s/2,7,141 + ($s+1)/2,13);
				}
				else if (%selected == 2) {
					InvertRect(68,27,119,33);
				}
			}//*/
		}
		else {
			%selected = -1;
		}
		/*
		if (size($p.spectrum)) {
			DrawSpectrum($p.spectrum, 0, 106, 319, 239);
		}
		else if (size($p.waveform)) {
			DrawWave($p.waveform, 0, 106, 319, 239);
		}//*/
		/*
		$mapping = OpenFileMapping("VLC_SVIDEOPIPE");
		$view = $mapping.MapViewOfFile(0, 640*480*4+64);
		$headerSize = $view.ReadInt(0, 4);
		$res2 = $view.ReadInts(7*4, 2, 1, 2);
		$bpp = $view.ReadInt(8*4, 1);
		$image = $view.LoadImage($res2[0], $res2[1], $bpp, $headerSize);
		DrawImage($image, ($res[0]-$res2[0])/2, 106);
		//*/
	}

	function Draw($event, $param, $name, $res) {
		ClearScreen();
		/* Too fast, but fits in with my scrolling rate. ~2 per second
		 * gives reasonable second update rate.
		 */
		$p = %players[%player];
		// Tell it that it's been drawn, so knows when it can kill itself if it has
		// an open socket and is idle.
		$p.Drawn();

		if (%bump >= 6) {
			$p.Update();
			%bump %= 6;
		}

		if (IsHighRes(@$res))
			%DrawG19(@$);
		else
			%DrawG15();
	}
};
