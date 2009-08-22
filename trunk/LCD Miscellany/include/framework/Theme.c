// Designed to allow changing theme at run time, though currently nothing
// in the front end that allows it.

function LoadTheme($theme) {
	$reader = FileReader("themes\" +s $theme, READER_CONVERT_TEXT);//"

	gLoadedFonts = list();

	gThemeData = $reader.Read(100*1024);

	colorBg = GetThemeColor("color", 0, 0);
	colorText = GetThemeColor("color", 1, 0);

	colorHighlightBg = GetThemeColor("colorHighlight", 0, 1);
	colorHighlightText = GetThemeColor("colorHighlight", 1, 1);

	// Only used on colored screens, so don't need to be careful.
	colorHighlightUnfocusedBg = GetThemeColor("colorHighlightUnfocused", 0, 1);
	colorHighlightUnfocusedText = GetThemeColor("colorHighlightUnfocused", 1, 1);

	// Used in multi-column displays to make columns easier to read.
	colorFirstColumnBg = GetThemeColor("colorFirstColumn", 0, 0);
	// Even/odd column color options.
	colorColumnBg = list(
						GetThemeColor("colorEvenColumn", 0, 0),
						GetThemeColor("colorOddColumn", 0, 0)
	);

	// currently only used by task manager.  May add elsewhere eventually.
	colorScrollBar = GetThemeColor("colorScrollBar", 1, 0);
}

function GetThemeString($name) {
	return RegExp(gThemeData, "\n\w*" +s $name +s "\w*=\w*(.*)")[0][0];
}

function GetThemeColor($name, $text, $inverted) {
	if ($text) {
		$name +=s "Text";
	}
	else {
		$name +=s "Bg";
	}
	$string = GetThemeString($name);

	if (!size($string)) {
		if ($inverted) {
			if ($text) {
				$string = GetThemeString("colorDefaultText");
			}
			else {
				$string = GetThemeString("colorDefaultBg");
			}
		}
		else {
			if ($text) {
				$string = GetThemeString("colorDefaultInvertedText");
			}
			else {
				$string = GetThemeString("colorDefaultInvertedBg");
			}
		}
	}

	$rgba = strsplit($string, ",", 0, 0, 0);
	if (size($rgba) > 3)
		return RGBA(@$rgba);
	else
		return RGB(@$rgba);
}

function LoadThemeFont($name) {
	$string = GetThemeString($name);

	// Internal font, hopefully
	if (size($string) == 1) {
		return $string + 0;
	}
	return Font(@strsplit($string, ",", 0, 0, 0));
}

function RegisterThemeFont($name) {
	if (!size(gFontNames)) {
		gFontNames = list();
	}
	for ($id = 0; $id<size(gFontNames); $id++) {
		if (gFontNames[$id] ==S $name)
			return $id;
	}
	gFontNames[$id] = $name;
	return $id;
}

function RegisterThemeFontPair($name) {
	return list(
		RegisterThemeFont("small" +s $name),
		RegisterThemeFont("big" +s $name)
	);
}

function GetThemeFont($id) {
	if (!size(gLoadedFonts[$id])) {
		$first = gFontNames[$id][0];
		// Load all fonts with the same first letter.
		// Result is that will load all big/small fonts when only need one,
		// so when change screen, won't have to load more fonts, which would cause annoying
		// delays later on.
		for ($i = size(gFontNames)-1; $i>=0; $i--) {
			if (!size(gLoadedFonts[$i]) && gFontNames[$i][0] ==S $first) {
				gLoadedFonts[$i] = LoadThemeFont(gFontNames[$i]);
			}
		}
	}
	return gLoadedFonts[$id];
}

// Here mostly because it affects choice of theme settings.
// Currently ignore bpp, though images basically assume color on highres screens.
function IsScreenHighRes($w, $h, $bpp) {
	// return ($bpp >= 24 && $w >= 320);
	return ($w >= 320);
}
