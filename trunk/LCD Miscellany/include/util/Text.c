#import <constants.h>

/* A couple test related utility functions and objects.
 */

function MediaFlip($vk) {
	if (!size(GetG15s(LCD_G15_V1))) {
		if ($vk == VK_VOLUME_DOWN) return VK_VOLUME_UP;
		if ($vk == VK_VOLUME_UP) return VK_VOLUME_DOWN;
	}
	return $vk;
}

/* Works just like RegisterKeyEvent, only doesn't require all values individually.
 */
function RegisterKeyRange($modifiers, $min, $max) {
	$values = list();
	// Backwards is slightly faster - list size is increased only the first time.
	for ($i=$max; $i>=$min; $i--) {
		$values[$i - $min] = $i;
	}
	return RegisterKeyEvent($modifiers, @$values);
}

/* Works just like UnregisterKeyEvent, only doesn't require all values individually.
 */
function UnregisterKeyRange($modifiers, $min, $max) {
	$values = list();
	for ($i=$max; $i>=$min; $i--) {
		$values[$i - $min] = $i;
	}
	return UnregisterKeyEvent($modifiers, @$values);
}

// For compatibilty with old scripts...  Oops.
function Elipsisify() {
	Ellipsisify(@$);
}

function Ellipsisify($text, $width) {
	if (TextSize($text)[0] <= $width || size($text)<=3) return $text;
	$w = $s = TextSize("...")[0];
	while ($w < $width) {
		$len = $next;
		$next = $len+1;
		while ($text[$next] >=s "|x80") $next++;
		$w = $s + TextSize(substring($text, 0, $next+1))[0];
	}
	return substring($text, 0, $len+1) +s "...";
}

function EllipsisifyPath($text, $width) {
	if (TextSize($text)[0] <= $width || size($text)<=3) return $text;
	$path = strsplit($text, "\");//"
	if (size($path) <= 2) return Elipsisify($text, $width);

	$prefix = $path[0] +s "\...\";//"

	$index = size($path) - 1;
	$suffix = $path[$index];
	$index--;

	$s = TextSize($prefix)[0];
	while ($index) {
		$suffix2 = $path[$index] +s "\" +s $suffix;//"
		$w = $s + TextSize($suffix2)[0];
		if ($w >= $width) break;
	}
	// May have to use two Ellipses with really long directory names.
	if ($index)	return Elipsisify($prefix +s $suffix, $width);
	// Shouldn't happen, unless there are a lot of extra backslashes (Need at least 3 or 4).
	return $path[0] +s "\" +s $suffix;//"
}

// Doesn't handle overdraw on either side of text...Yet.
struct ScrollingText {
	// Safe to set externally.
	var %width;
	// Cached.
	var %textWidth, %lastTime;
	var %interval, %shift, %center;

	var %offset, %text, %overflow;
	function ScrollingText($ntext, $nwidth, $nshift, $ninterval, $ncenter, $noverflow) {
		%width = $nwidth;
		%shift = $nshift;
		%interval = $ninterval;
		%center = $ncenter;
		%overflow = $noverflow;
		if (!$noverflow)
			%overflow = %shift;
		%SetText($ntext);
	}

	function SetText($ntext) {
		if (%text !=S $ntext) {
			%text = $ntext;
			%textWidth = TextSize($ntext)[0];
			%offset = -%textWidth-2*%shift;
			%lastTime = GetTickCount() - %interval;
		}
	}

	function DisplayText($x, $y) {
		if (%width < %textWidth) {
			$test = GetTickCount() - %lastTime;
			if ($test >= %interval/2) {
				$test2 = ($test+%interval/2)/%interval;
				if ($test2 >= 2) {
					$test2 = $test/%interval;
				}
				%lastTime += $test2 * %interval;
				%offset -= $test2 * %shift;
				if (%offset+%overflow + %textWidth < %width) {
					%offset = 0;
				}
			}
		}
		else {
			if (%center) {
				DisplayTextCentered(%text, $x + %width/2, $y);
				return;
			}
			%offset = 0;
		}
		DisplayText(%text, $x + %offset, $y);
	}
};

// Easier to move steadily, but have to do more work.
struct ManualScrollingText {
	// Safe to set externally.
	var %width;
	// Cached.
	var %textWidth;
	var %shift, %center;

	var %offset, %text, %overflow;

	function %Bump() {
		%offset -= %shift;
		if (%offset+%overflow + %textWidth < %width) {
			%offset = %overflow;
		}
	}

	function ManualScrollingText($ntext, $nwidth, $nshift, $ncenter, $noverflow) {
		%width = $nwidth;
		%shift = $nshift;
		%center = $ncenter;
		%overflow = $noverflow;
		if (!$noverflow)
			%overflow = %shift;
		%SetText($ntext);
	}

	function SetText($ntext) {
		if (%text !=S $ntext) {
			%text = $ntext;
			%textWidth = TextSize($ntext)[0];
			%offset = %overflow;
		}
	}

	function DisplayText($x, $y) {
		if (%width < %textWidth) {
		}
		else {
			if (%center) {
				DisplayTextCentered(%text, $x + %width/2, $y);
				return;
			}
			%offset = %overflow;
		}
		$pos = %offset;
		if ($pos > 0) $pos = 0;
		else if ($pos + %textWidth < %width)
			$pos = %width - %textWidth;
		DisplayText(%text, $x + $pos, $y);
	}
};

