#requires <framework\Theme.c>

function DisplayHeader($res) {
	$width = $res[0];
	if (!gBigHeaderFont) {
		gSmallHeaderFont = RegisterThemeFont("smallHeaderFont");
		gBigHeaderFont = RegisterThemeFont("bigHeaderFont");
		gEmailImage = LoadImage("images\email.png");
	}
	if ($width <= 160) {
		UseThemeFont(gSmallHeaderFont);
		ClearRect(0,0,159,6);
		DisplayTextRight(FormatTime("DDD M.DD.YY"),160,0);
		DisplayTextRight(FormatTime("hh:NN:SS TT"), 57);

		// Pretty blatant copy of SirReal's
		if (GetEmailCount()) {
			if (IsNull(emailImage)) emailImage = LoadImage("images\email.png");
			DrawImage(gEmailImage, 76, 0);
		}

		DrawRect(0, 7, 159, 7);
	}
	else {
		UseThemeFont(gBigHeaderFont);
		if (IsNull(titleBarImage)) titleBarImage = LoadImage32("images\TitleBar.png");
		//ColorRect(0, 0, $width, 20, colorHighlightBg);
		DrawImage(titleBarImage);
		SetDrawColor(colorHighlightText);
		DisplayTextRight(FormatTime("DDD M.DD.YY"),$width-3,0);
		DisplayTextRight(FormatTime("hh:NN:SS TT"), TextSize("hh:NN:SS TT")[0]);
		/*if (GetEmailCount()) {
			DrawImage(gEmailImage, $width/2-8, 4);
			DisplayText(GetEmailCount(), $width/2+2);
		}//*/
		SetDrawColor(colorText);
	}
}
