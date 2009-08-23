#import <Everything.h>
// Note:  For editing this file, I highly recommend an editor that
// colors code.  TextPad is what I use, though it's not free.  There's
// a free version of EmEditor, which has nice unicode support as well
// as coloring code.  There's also a demo version of the non-free version,
// which is easy to accidentally download.

function Init() {
	/* Global colors.  May move them elsehwere in the future.
	 */

	LoadTheme("Default.txt");

	/* Fonts list.  Here so it's easier to change them as desired.
	 * Only fonts used for text-only screens are here.  Other screens have to have item placement
	 * modified when changing font, so they're not here.
	 */

	// Fonts for small (160 by 43 monochrome LCDs
	// Primary internal font.  It's the larger of the two small all caps fonts.
	$smallTaskManagerFont = 0;
	$smallCalculatorFont = Font("04b03", 8);
	$smallUtorrentFont = Font("04b03", 8);
	$smallClipboardFont = Font("04b03", 8);
	$smallRssFont = Font("04b03", 8);
	$smallRssTitleFont = Font("04b03", 8, 0, 1);

	// Fonts for large (320 by 160) color LCDs.  Can get rid of these lines to save some
	// memory if you don't have one.
	// Probably have to switch them all to bold, but not sure.
	$bigTaskManagerFont = Font("Arial", 18,0,0,0,CLEARTYPE_QUALITY);
	$bigCalculatorFont = Font("Arial", 24,0,0,0,CLEARTYPE_QUALITY);
	$bigUtorrentFont = Font("Arial", 18,0,0,0,CLEARTYPE_QUALITY);
	$bigClipboardFont = Font("Arial", 18,0,0,0,CLEARTYPE_QUALITY);
	$bigRssFont = Font("Arial", 18,0,0,0,CLEARTYPE_QUALITY);
	$bigRssTitleFont = Font("Arial", 18,0,1,0,CLEARTYPE_QUALITY);

	/* Uncomment to use US english day/month strings instead of whatever Windows
	 * is configured to use.
	 */
	// SetLocale(0x0409, 1);

	/* Enable low-level communication with the G15.  Makes playing with lights faster,
	 * allows V1/V2 differentiation, enables display when LCDMon.exe is not running,
	 * allows g- and m-key handling, etc.  Does not break compatibility when LCDMon is
	 * running.
	 */
	G15EnableHID();

	/* Will steal LCD when started.  Remove/comment out line to change that.
	 * Was added in the hope that it would cause the G19 to display LCD Miscellany instead
	 * of the app list.  Unfortunately, doesn't do that.  Note that this will make the first
	 * update only high priority.  Will return to normal afterwards.
	 */
	G15SetPriority(LGLCD_PRIORITY_ALERT);
	eventHandler = EventHandler();

	/* Change order (or comment out) to change order of views (or hide them)
	 */
	menuHandler = MenuHandler(eventHandler,
		DualView (
			StatusView(
				//DefaultStatus(),
				QuadStatus(),
				GameStatus(),
				// Sample screen with speedfan info.  Useful for figuring out which
				// number is what.
				// SpeedFanStatus(),
			),
			StatusView(
				G19Status(),
			)
		),

		// Change 0 to 1 to disable stealing non-media keys when focused.
		TaskManager(0),

		// Modular chat.  Currently only module is IRC.  If Pidgin leak is ever fixed, I'll modify the pidgin script
		// to be compatible.  Designed for small 2-color LCDs only.
		/*
		ChatView(
			IRCChatService(list("irc.goatcheese.cx:7000", "CheeseLover", "#CheeseChat", "#CheeseWarehousing", "#YouGetTheIdea")),
		),
		//*/

		// Doesn't add anything unless Pidgin is configured in the ini.
		GetPidginView(),

		MediaView(
			MPCController(),
			VLCController(),
			WinampController(),
			ItunesController(),
			// Really, really slow, probably because of DDE.
			// AMIPController(),
		),

		TextEditorView(),
		ClipboardView(),

		CalculatorView(),

		// Specify urls like the next line, or modify ini:
		// RSSView(RSS_DISPLAY_TITLE | RSS_DISPLAY_DESCRIPTION,RSS_DISPLAY_TITLE | RSS_DISPLAY_DESCRIPTION,"http://news.google.com/?rss=&output=rss"),

		RSSView(
			RSS_DISPLAY_TITLE | RSS_DISPLAY_DESCRIPTION, // Channel title flags
			RSS_DISPLAY_TITLE | RSS_DISPLAY_DESCRIPTION, // Item text flags
			, // URL to use.  Can either be a string or a list of strings.  If left null, uses one in ini.
			 // Channels to display.  Currently, can only specify one per entry in previous parameter.
		),

		// uTorrent.
		DownloadView($smallUtorrentFont, $bigUtorrentFont),

		// SABnzbdView 0.1.7 by 4wd:
		// Designed for small 2-color LCDs only.
		// Parameters are: SABnzbdView(URL, header, subview),
		// Where: URL = "http://ip:port/sabnzbd/"
		//     header = 0 if you want the TIME/DATE header
		//              1 if you want IP:Port header
		//    subview = 0 if not using StatusView for multiple views
		//              1 if using StatusView (allows Pause/Resume/Stop)
		//
		// For SABnzbd+ 0.4.9 or higher the URL must be:
		//        URL = "http://ip:port/sabnzbd/api?apikey=<insert key>"
		//
		// If you are only interested in one SABnzbd+ daemon, you can leave the
		// URL empty and place it in the LCDMisc.ini under [URLs].  See the example.
		// SABnzbdView(, 0, 0),

		// The sample below shows using StatusView to monitor more than one SABnzbd+
		// Switching between screens is done by pressing the LCDMisc 'OK' button,
		// (third LCD button).  URL is required when using StatusView.
		// You can mix URLs for SABnzbd+ versions.
		/*
			StatusView("Images\SABnzbd.png",
			  SABnzbdView("http://1.2.3.4:8080/sabnzbd/",1, 1),
			  SABnzbdView("http://5.6.7.8:8080/sabnzbd/api?apikey=77e45017",1, 1)
			),
		*/


		/* Parameters aren't needed, but replace the 0 with a 1 to display location
		 * instead of time at the top, over the header.  No room for both,
		 * unfortunately.  Could squeeze the location and either the date or time on
		 * together for most location names, but not all, so sticking with either-or
		 * for now.  May switch to a smaller forecast ticker to squeeze everything
		 * on, or separate current weather/forecast screens.
		 */
		WeatherView(,0),

		// Doesn't work yet...  Some day...
		// May already exist, so don't call constructor.
		// GetConfigView(),

		// Sample showing how to use a StatusView to let you switch between
		// weather screens.  Note that the first is in Fahrenheit and second
		// is in Celsuis.  Just uncomment (Delete  the "/*") and change/add
		// urls as needed.
		/*
		StatusView("Images\Weather.png",
			WeatherView("http://xml.weather.yahoo.com/forecastrss?p=usma0066&u=f", 1),
			WeatherView("http://xml.weather.yahoo.com/forecastrss?p=CAXX0487&u=c", 1)
		),
		//*/

		// Probably will never get around to getting this working, due to threading issues.
		//CommandView(),
	);

	// Turn off lights during screensaver.  Delete the "//" to enable.
	ScreenSaverLightToggle(eventHandler);

	/* Smoothes out system volume changes.  Note that it doesn't steal the
	 * volume wheel.  Instead, it monitors the system volume setting.
	 * It will partially break the Windows Master volume slider.
	 * Intended to rectify the default ~4% adjustment per tick of the
	 * volume wheel, which is much too coarse if you use a lower setting.
	 * Experimental, breaks dragging on master volume control.  Vista
	 * already uses smaller increments, so not useful there.
	 */
	//VolumeSmoother(eventHandler);

	// Tray icons.  Uncomment whichever ones you like.
	//AddCpuIcon();
	//AddTotalCpuIcon();
	//AddNetIcon();
}

