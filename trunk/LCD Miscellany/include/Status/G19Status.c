#requires <framework\header.c>
#requires <util\CPUSaver.c>
#requires <util\Audio.c>
#import <constants.h>
#import <Modules\Graph.c>
#requires <Modules\DriveInfo.c>
#requires <Modules\CounterManager.c>

struct G19Status {
	var %miniFont, %font, %counters;
	function G19Status() {
		%miniFont = Font("6px2bus", 6);
		%counters = GetCounterManager();
		%font = Font("Arial", 20,0,0,0,CLEARTYPE_QUALITY);
    }

	function Draw($event, $param, $name, $res) {
		ClearScreen();

		DisplayHeader($res);

		UseFont(%font);

		/*

		DrawRect(86, 7, 86, 43);
		DrawRect(58, 7, 58, 43);
		//*/

		DisplayText("CPU:", 1, 23);
		DisplayTextRight(FormatValue(%counters.cpuData["_Total"], 0, 1), 85, 23);

		$memory = GetMemoryStatus();

		DisplayTextRight("MEM:", 41, 42);
		DisplayTextRight(FormatValue($memory[1]/1024.0/1024.0) +s " M", 102, 42);
		DisplayTextRight("VM:", 41, 60);
		DisplayTextRight(FormatValue($memory[3]/1024.0/1024.0) +s " M", 102, 60);

		DisplayText("Vol:", 1, 80);
		$volume = GetMaxAudioState(0);
		$vString = FormatValue($volume[0]) +s "%";
		DisplayTextRight($vString, 80, 80);

		if ($volume[1]) {
			DrawRect(80-TextSize($vString)[0], 87, 80, 87);
		}

		DisplayDriveInfo(190,23,318,84, -3);
		DisplayTextCentered(GetRemoteIP(), 240, 77);

		DisplayText("Dn:", 162, 95);
		DisplayTextRight(FormatSize(%counters.down, 1,0,1), 232, 95);
		DisplayText("Up:", 248, 95);
		DisplayTextRight(FormatSize(%counters.up, 1,0,1), 318, 95);

		DrawMultiGraph(%counters.cpuGraph, 0, 239, 158, 116,,list(0,2));
		DrawMultiGraph(list(%counters.downGraph, %counters.upGraph), 161, 239, 319, 116,,list(0,2));
	}
};
