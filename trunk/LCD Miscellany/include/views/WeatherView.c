#import <Views\View.c>
#requires <framework\header.c>
#requires <util\XML.c>

struct WeatherView extends View {
	var %weatherImage,
		%colorWeatherImage,
		%wind,
		%windChill,
		%weatherDescription,
		%humidity,
		%temperature,
		%units,

		%forecast,

		%offset,

		%scrollTimer,

		%url,

		%location,

		%lastUpdate,

		%displayLocation,

		%miniFont,

		%bigFont;

	function WeatherView ($_url, $_location) {
		%lastUpdate = Time()-60*60-5;

		%miniFont = Font("6px2bus", 6);

		%bigFont = Font("Arial", 18,0,0,0,CLEARTYPE_QUALITY);

		%url = $_url;
		if (!size(%url))
			%url = GetString("URLs", "Weather");
		//%url = "http://127.0.0.1/forecastrss.xhtml";
		if (IsNull(stristr(%url, "u=c"))) {
			%toolbarImage = LoadImage("Images\Weather.png");
		}
		else {
			%toolbarImage = LoadImage("Images\Weatherc.png");
		}

		// Draw on counter updates and after 6 second intervals for scrolling.
		// Means more redraws than are really needed, but scrolling is too fast at 1 second,
		// too slow at 2 second, and need to update the time display, too.
		%noDrawOnCounterUpdate = 0;
		%noDrawOnAudioChange = 1;

		%displayLocation = $_location;
	}

	function Show() {
		if (!%scrollTimer) {
			%offset = 0;
			%scrollTimer = CreateTimer("Bump", 6,,$this);
		}
	}

	function Hide() {
		if (%scrollTimer) {
			KillTimer(%scrollTimer);
			%scrollTimer = 0;
		}
	}

	function Bump() {
		NeedRedraw();
		if (size(%forecast) > 2) {
			%offset += 28;
			if (%offset < 28*3) return;
		}
		%offset = 0;
	}

	// Update using Yahoo Weather.
	function Update() {
		$xml = HttpGetWait(%url);
		if (IsNull($xml)) {
			// Update every 5 minutes when server's down.
			%lastUpdate = Time()-55*60;
		}
		else {
			$xml = SimpleXML(ParseXML($xml), ("rss", "channel"));
			$item = SimpleXML($xml, list("item"));
			%wind = SimpleXML($xml, ("yweather:wind", , "speed")) +s " " +s SimpleXML($xml, ("yweather:units", , "speed"));

			%windChill = SimpleXML($xml, ("yweather:wind", , "chill"));
			%weatherDescription = SimpleXML($item, ("yweather:condition", , "text"));
			%humidity = SimpleXML($xml, ("yweather:atmosphere", , "humidity"));
			%units = SimpleXML($xml, ("yweather:units", , "temperature"));

			%location = SimpleXML($xml, ("yweather:location", , "city")) +s ", " +s SimpleXML($xml, ("yweather:location", , "region"));

			if (%humidity >= 40 && (%units ==S "F" && %temperature >= 80) || (%units ==S "C" && %temperature >= (80-32)*5/9.0)) {
				if (%units ==S "C") {
					%temperature = %temperature * (9/5.0) + 32;
				}
				// Actually heat index.
				%windChill =
					- 42.379
					+ 2.04901523*%temperature
					+10.14333127*%humidity
					- 0.22475541*%temperature*%humidity
					- 0.00683783*%temperature*%temperature
					- 0.05481717*%humidity*%humidity
					+ 0.00122874*%temperature*%temperature*%humidity
					+ 0.00085282*%temperature*%humidity*%humidity
					- 0.00000199*%temperature*%temperature*%humidity*%humidity;
				if (%units ==S "C") {
					%temperature = (%temperature - 32)*(5/9.0);
					%windChill = (%windChill - 32)*(5/9.0);
					%temperature = FormatValue(%temperature);
				}
				%windChill = FormatValue(%windChill);
			}
			%temperature = SimpleXML($item, ("yweather:condition", , "temp"));

			%forecast = list();

			for ($i=0; $i<5; $i++) {
				$index = FindNextXML($item, "yweather:forecast", $index+1);
				$data = $item[$index+1][1];
				if (IsNull($data)) break;
				%forecast[$i] = (SimpleXML($data, list("day")),
								 SimpleXML($data, list("low")),
								 SimpleXML($data, list("high")),
								 SimpleXML($data, list("text"))
				);
			}

			// Call before getting image, so can redraw while getting img.
			NeedRedraw();

			$html = SimpleXML($item, list("description"))[3];
			if (size($html)) {
				$html = ParseXML($html);
				if (!IsNull($html)) {
					$img = SimpleXML($html, ("img", ,"src"));
					if (!IsNull($img)) {
						$img = HttpGetWait($img);
						if (!IsNull($img)) {
							%weatherImage = LoadMemoryImage($img, 0.6);
							%colorWeatherImage = LoadMemoryImage32($img);
							NeedRedraw();
						}
					}
				}
			}
		}
	}

	function DrawG15() {
		UseFont(0);
		if (IsNull(%temperature)) {
			DisplayText("No weather data.", 0, 8);
		}
		else {

			DisplayText("Temp: " +s %temperature, 0, 8);
			DisplayText("Index: " +s %windChill, 0, 15);
			DisplayText(%wind, 0, 22);

			DisplayText("Humid: " +s %humidity, 0, 29);

			DrawImage(%weatherImage, 49, 8);

			// Clear pixels jsut about description.
			ClearRect(49,37,79,42);

			if (TextSize(%weatherDescription)[0] > 80) {
				$voffset = 1;
				UseFont(%miniFont);
			}
			DisplayText(%weatherDescription, $voffset, 37);
			UseFont(0);

			ClearRect(79,37,159,42);

			DrawRect(80, 7, 80, 42);

			$realOffset = %offset;

			for (; $i<size(%forecast); $i++) {
				$height = 8 + 14 * $i - $realOffset;
				$day = %forecast[$i];
				DisplayText($day[0],82,$height);
				DisplayTextRight($day[1] +s " -",135,$height);
				DisplayTextRight($day[2],159,$height);
				InvertRect(81, $height, 159, $height+6);
				if (TextSize($day[3])[0] > 79) {
					UseFont(%miniFont);
					$height++;
				}
				DisplayText($day[3],82,$height+7);
				UseFont(0);
			}
		}
		if (!%displayLocation)
			DisplayHeader();
		else {
			ClearRect(0,0,159,6);
			DisplayText(%location,0,0);
			DrawRect(0, 7, 159, 7);
		}
	}

	function DrawG19($event, $param, $name, $res) {
		UseFont(%bigFont);
		DisplayHeader($res);
		if (IsNull(%temperature)) {
			DisplayText("No weather data.", 0, 8);
		}
		else {
			$deg = "°" +s %units;
			DisplayText(%location +s " weather", 1, 20);
			ColorRect(0,44,319,46, colorHighlightBg);
			DisplayText("Temp: " +s %temperature +s $deg, 1, 46);
			DisplayText("Index: " +s %windChill +s $deg, 1, 66);
			DisplayText(%wind, 1, 86);
			DisplayText("Humidity: " +s %humidity, 1, 106);

			DrawImage(%colorWeatherImage, 180, 53);
			DisplayTextCentered(%weatherDescription, 206, 103);

			ColorRect(0,129,319,150, colorHighlightBg);
			SetDrawColor(colorHighlightText);
			DisplayText("Forecast", 4, 129);
			SetDrawColor(colorText);

			for ($i=0; $i<2; $i++) {
				$x = 160*$i+2;
				$day = %forecast[$i];
				DisplayText($day[0],$x,151);
				DisplayText($day[1] +s $deg +s " - " +s $day[2] +s $deg,$x, 171);
				DisplayText($day[3],$x,191);
			}
/*
			DisplayText("Temp: " +s %temperature, 0, 8);
			DisplayText("Index: " +s %windChill, 0, 15);
			DisplayText(%wind, 0, 22);


			DrawImage(%weatherImage, 49, 8);

			// Clear pixels jsut about description.
			ClearRect(49,37,79,42);

			if (TextSize(%weatherDescription)[0] > 80) {
				$voffset = 1;
				UseFont(%miniFont);
			}
			DisplayText(%weatherDescription, $voffset, 37);
			UseFont(0);

			ClearRect(79,37,159,42);

			DrawRect(80, 7, 80, 42);

			$realOffset = %offset;

			for (; $i<size(%forecast); $i++) {
				$height = 8 + 14 * $i - $realOffset;
				$day = %forecast[$i];
				DisplayText($day[0],82,$height);
				DisplayTextRight($day[1] +s " -",135,$height);
				DisplayTextRight($day[2],159,$height);
				InvertRect(81, $height, 159, $height+6);
				if (TextSize($day[3])[0] > 79) {
					UseFont(%miniFont);
					$height++;
				}
				DisplayText($day[3],82,$height+7);
				UseFont(0);
			}
			//*/
		}
	}

	function Draw($event, $param, $name, $res) {
		ClearScreen();

		if (Time()-%lastUpdate >= 60*60) {
			%lastUpdate = Time();
			SpawnThread("Update", $this);
		}

		if ($res[0] >= 320) %DrawG19(@$);
		else %DrawG15();

	}
};
