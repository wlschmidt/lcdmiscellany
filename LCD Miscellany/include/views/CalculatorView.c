#import <Views\View.c>
#requires <framework\header.c>
#requires <Modules\LineEditor.c>

function EvalSub($vals) {
	while (size($vals) > 1) {
		$s = size($vals);
		$found = 1;

		for ($i = 0; $i<size($vals); $i++) {
			if ($vals[$i] ==s "(") {
				for ($j = $i+1; $j<size($vals); $j++) {
					if ($vals[$j] ==s "(" || $vals[$j] ==s ")") break;
				}
				if ($vals[$j] ==s ")") {
					if ($j-1 == $i) {
						$rep = null;
					}
					else {
						$rep = EvalSub(pop_range ($vals, $i+1, $j-1));
						if ($rep ==s "Error") return "Error";
					}
					pop_range ($vals, $i, $i);
					$vals[$i] = $rep;
					if (IsString($vals[$i-1]) && IsNull(RegExp($vals[$i-1], "^[-*+^/\(\)]$"))) {
						$vals[$i-1] = call($vals[$i-1], list($vals[$i]));
						pop_range ($vals, $i, $i);
						if (IsNull($vals[$i-1])) return "Error";
					}
					break;
				}
			}
		}
		if ($s != size($vals)) continue;

		for ($i = size($vals)-3; $i>=0; $i--) {
			if ($vals[$i+1] ==s "^" && IsNumber($vals[$i]) && IsNumber($vals[$i+2])) {
				$vals[$i] = pow($vals[$i], $vals[$i+2]);
				pop_range($vals, $i+1, $i+2);
			}
		}

		for ($i = size($vals)-3; $i>=-1; $i--) {
			if ($vals[$i+1] ==s "-" && IsNumber($vals[$i+2]) && !IsNumber($vals[$i])) {
				$vals[$i+2] = -$vals[$i+2];
				pop_range($vals, $i+1, $i+1);
				$i--;
			}
		}

		for ($i = 0; $i < size($vals)-2; $i++) {
			if (IsNumber($vals[$i]) && IsNumber($vals[$i+2])) {
				if ($vals[$i+1] ==s "*") {
					$vals[$i] = $vals[$i] * $vals[$i+2];
					pop_range($vals, $i+1, $i+2);
					$i--;
				}
				else if ($vals[$i+1] ==s "/") {
					$v1 = $vals[$i];
					$v2 = $vals[$i+2];
					if ($v2 * ($v1/$v2) != $v1) {
						$v1 += 0.0;
					}
					$vals[$i] = $v1 / $v2;
					pop_range($vals, $i+1, $i+2);
					$i--;
				}
			}
		}

		for ($i = 0; $i < size($vals)-2; $i++) {
			if (IsNumber($vals[$i]) && IsNumber($vals[$i+2])) {
				if ($vals[$i+1] ==s "+") {
					$vals[$i] = $vals[$i] + $vals[$i+2];
					pop_range($vals, $i+1, $i+2);
					$i--;
				}
				else if ($vals[$i+1] ==s "-") {
					$vals[$i] = $vals[$i] - $vals[$i+2];
					pop_range($vals, $i+1, $i+2);
					$i--;
				}
			}
		}
		break;
	}
	if (size($vals) == 1 && IsNumber($vals[0])) return $vals[0];
	return "Error";
}

function Eval($s, $previous) {
	$vals = list();
	if (!IsNull(RegExp($s, "^[ |t]*[+*/^]")) || !IsNull(RegExp($s, "^-"))) {
		$vals[0] = $previous;
	}
	$s = strreplace($s, " ");
	$s = strreplace($s, "|t");
	while (size($s)) {
		if (!IsNull($test = RegExp($s, "^(0[xX][0-9a-fA-F]+)"))
			 /*|| !IsNull($test = RegExp($s, "^([0-9a-fA-F]+[hH])"))//*/) {
					$vals[size($vals)] = $test[0][0]+0.0;
					$s = substring($s, size($test[0][0]), -1);
		}
		else if (!IsNull($test = RegExp($s, "^([0-9]*\.[0-9]+)")) ||
				 !IsNull($test = RegExp($s, "^([0-9]+\.?)"))) {
					$vals[size($vals)] = $test[0][0]+0.0;
					$s = substring($s, size($test[0][0]), -1);
		}
		else if (!IsNull($test = RegExp($s, "^([_a-zA-Z][_a-zA-Z0-9]*)"))) {
					$vals[size($vals)] = $test[0][0];
					$s = substring($s, size($test[0][0]), -1);
		}
		else if (!IsNull($test = RegExp($s, "^([(,)])"))) {
					$vals[size($vals)] = $test[0][0];
					$s = substring($s, 1, -1);
		}
		else if (size($s) == 0) {
			break;
		}
		else if (IsNull($test = RegExp($s, "^([-+*/^])"))) {
			return "Error";
		}
		else {
			$vals[size($vals)] = $test[0][0];
			$s = substring($s, 1, -1);
		}
	}
	return EvalSub($vals);
}

struct CalculatorView extends View {
	var %buffer, %bufferPos, %editor;
	// current editor's width and font.  Minor optimization.
	var %width, %activeFont;

	function CalculatorView () {
		%InitFonts();
		%buffer = list();
		%toolbarImage = LoadImage("Images\Calculator.png");
		%editor = LineEditor(160, %activeFont=0);
		%noDrawOnCounterUpdate = 0;
		%noDrawOnAudioChange = 1;
	}

	function Focus() {
		if (!%hasFocus) {
			RegisterKeyEvent(3<<16, VK_ESCAPE);
			%editor.Focus();
			%hasFocus = 1;
		}
	}

	function KeyDown($event, $param, $modifiers, $vk, $string) {
		NeedRedraw();
		if ($vk == VK_ESCAPE) {
			%Unfocus();
		}
		else if ($vk == VK_RETURN) {
			$text = %editor.GetText();
			// Repeat last operation.
			if (!size($text)) $text = %buffer[size(%buffer)-2];
			if (size($text) >= 1) {
				push_back(%buffer, $text);
				push_back(%buffer, Eval($text, %buffer[size(%buffer)-2]));
				%editor.Clear();
				if (size(%buffer) > 250) {
					pop_range(%buffer, 0, 1);
				}
				%bufferPos = size(%buffer);
			}
		}
		else if ($vk == VK_UP || $vk == VK_DOWN) {
			$dp = -1;
			if ($vk == VK_DOWN)
				$dp = 1;
			$p = %bufferPos;
			while (1) {
				$p += $dp;
				if ($p == size(%buffer)) {
					%editor.Clear();
					%bufferPos = $p;
					return 1;
				}
				// Run off the top.
				if (size(%buffer[$p]) == 0) {
					return 1;
				}
				if (%buffer[$p] !=s "Error") break;
			}
			/* May add current item to the buffer here, eventually, though I'd have to
			 * split the display buffer and the previous text buffer.
			 */
			//if ($vk == VK_UP && size(%editor.GetText()) && %bufferPos == size(%buffer) - 1) {
			//}
			%editor.SetText(%buffer[$p]);
			%bufferPos = $p;
		}
		else {
			return %editor.KeyDown(@$);
		}
		return 1;
	}

	function Unfocus() {
		if (%hasFocus) {
			UnregisterKeyEvent(3<<16, VK_ESCAPE);
			%hasFocus = 0;
			%editor.Unfocus();
		}
	}

	function G15ButtonDown($event, $param, $button) {
		$button = FilterButton($button);
		if ($button & 0x3F) {
			if ($button == G15_UP) {
				%KeyDown(,,,VK_UP);
			}
			else if ($button == G15_DOWN) {
				%KeyDown(,,,VK_DOWN);
			}
			else if ($button == G15_CANCEL) {
				%Unfocus();
			}
			NeedRedraw();
			return 1;
		}
	}

	function UpdateEditorAndFont($w, $highRes) {
		$index = IsHighRes(@$);
		// Allow more space to right.
		$w -= 4*$highRes;
		if (%width == $w) return;
		%width = $_w;
		%activeFont = GetThemeFont(%fontIds[$highRes]);
		%editor.ReFormat(%activeFont, $w);
	}

	function Draw($event, $param, $name, $res) {
		$w = $res[0];
		$h = $res[1]-1;

		$highRes = IsHighRes(@$res);
		%UpdateEditorAndFont($w, $highRes);

		ClearScreen();
		UseFont(%activeFont);
		$pos = size(%buffer)-1;
		$fh = GetFontHeight();
		$h2 = $h = $h - $fh - 1;
		DrawRect(0,$h,$w,$h);
		$L = 0;
		$R = $w;
		if ($highRes) {
			$R -= 3;
			$L  = 2;
		}
		$hpos = $h;
		while ($hpos > 0) {
			$hpos -= $fh;
			if ($pos & 1)
				DisplayTextRight(%buffer[$pos], $R, $hpos);
			else
				DisplayText(%buffer[$pos], $L, $hpos);
			$pos--;
			if ($pos < 0) break;
		}
		if ($hpos > $fh || $h > 100) {
			if (%hasFocus || $h < 100) {
				SetDrawColor(colorHighlightBg);
				DrawRect($w,$fh);
				SetDrawColor(colorHighlightText);
			}
			else {
				SetDrawColor(colorHighlightUnfocusedBg);
				DrawRect($w,$fh);
				SetDrawColor(colorHighlightUnfocusedText);
			}
			DisplayText("Calculator", $L+1);
			SetDrawColor(colorText);
		}

		%editor.Draw($L, $h2 + 1);
	}
};
